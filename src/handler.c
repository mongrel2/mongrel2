/**
 *
 * Copyright (c) 2010, Zed A. Shaw and Mongrel2 Project Contributors.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 *     * Neither the name of the Mongrel2 Project, Zed A. Shaw, nor the names
 *       of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written
 *       permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <handler.h>
#include <handler_parser.h>
#include <task/task.h>
#include "zmq_compat.h"
#include <dbg.h>
#include <stdlib.h>
#include <string.h>
#include <connection.h>
#include <assert.h>
#include <register.h>
#include "tnetstrings.h"
#include "xrequest.h"

#include "setting.h"

struct tagbstring LEAVE_HEADER_JSON = bsStatic("{\"METHOD\":\"JSON\"}");
struct tagbstring LEAVE_HEADER_TNET = bsStatic("16:6:METHOD,4:JSON,}");
struct tagbstring LEAVE_MSG = bsStatic("{\"type\":\"disconnect\"}");
struct tagbstring XREQ_CTL = bsStatic("ctl");
struct tagbstring KEEP_ALIVE = bsStatic("keep-alive");
struct tagbstring CREDITS = bsStatic("credits");
struct tagbstring CANCEL = bsStatic("cancel");

int HANDLER_STACK;

static void cstr_free(void *data, void *hint)
{
    (void)hint;

    free(data);
}

void Handler_notify_leave(Handler *handler, int id)
{
    void *socket = handler->send_socket;
    assert(socket && "Socket can't be NULL");
    bstring payload = NULL;

    if(handler->protocol == HANDLER_PROTO_TNET) {
        payload = bformat("%s %d @* %s%d:%s,",
                bdata(handler->send_ident), id,
                bdata(&LEAVE_HEADER_TNET),
                blength(&LEAVE_MSG), bdata(&LEAVE_MSG));
    } else {
        payload = bformat("%s %d @* %d:%s,%d:%s,",
                bdata(handler->send_ident), id,
                blength(&LEAVE_HEADER_JSON), bdata(&LEAVE_HEADER_JSON),
                blength(&LEAVE_MSG), bdata(&LEAVE_MSG));
    }

    check(payload != NULL, "Failed to make the payload for disconnect.");

    if(Handler_deliver(socket, bdata(payload), blength(payload)) == -1) {
        log_err("Can't tell handler %d died.", id);
    }

error: //fallthrough
    if(payload) free(payload);
}


int Handler_setup(Handler *handler)
{
    taskname("Handler_task");

    handler->task = taskself();

    handler->send_socket = Handler_send_create(bdata(handler->send_spec), bdata(handler->send_ident));
    check(handler->send_socket, "Failed to create handler socket.");

    handler->recv_socket = Handler_recv_create(bdata(handler->recv_spec), bdata(handler->recv_ident));
    check(handler->recv_socket, "Failed to create listener socket.");

    return 0;

error:
    return -1;

}

static inline void handler_cap_payload(bstring payload)
{
    if(payload->data[payload->slen - 1] == '\0') {
        btrunc(payload, blength(payload)-1);
    } else if(payload->data[payload->slen] != '\0') {
        bconchar(payload, '\0');
    }
}

static inline int deliver_payload(int raw, int fd, Connection *conn, bstring payload)
{
    int rc = 0;

    if(raw) {
        debug("Sending raw message to %d length %d", fd, blength(payload));
        rc = Connection_deliver_raw(conn, payload);
        check(rc != -1, "Error sending raw message to HTTP listener on FD %d, closing them.", fd);
    } else {
        debug("Sending BASE64 message to %d length %d", fd, blength(payload));
        handler_cap_payload(payload);
        rc = Connection_deliver(conn, payload);
        check(rc != -1, "Error sending to MSG listener on FD %d, closing them.", fd);
    }

    return 0;
error:
    return -1;
}

static int handler_process_control_request(Connection *conn, tns_value_t *data)
{
    tns_value_t *args = darray_get(data->value.list, 1);
    check(args->type==tns_tag_dict, "Invalid control response: not a dict.");

    hnode_t *n = hash_lookup(args->value.dict, &KEEP_ALIVE);
    if(n != NULL) {
        Register_ping(IOBuf_fd(conn->iob));
    }

    n = hash_lookup(args->value.dict, &CREDITS);
    if(n != NULL) {
        tns_value_t *credits = (tns_value_t *)hnode_get(n);
        conn->sendCredits += credits->value.number;
        taskwakeup(&conn->uploadRendez);
    }

    n = hash_lookup(args->value.dict, &CANCEL);
    if(n != NULL) {
        Register_disconnect(IOBuf_fd(conn->iob));
        taskwakeup(&conn->uploadRendez);
    }

    tns_value_destroy(data);
    return 0;

error:
    return -1;
}

static inline void handler_process_extended_request(int fd, Connection *conn, bstring payload)
{
    char *x;
    tns_value_t *data = NULL;
    darray_t *l = NULL;
    
    data = tns_parse(bdata(payload),blength(payload),&x);

    check((x-bdata(payload))==blength(payload), "Invalid extended response: extra data after tnetstring.");
    check(data->type==tns_tag_list, "Invalid extended response: not a list.");
    l = data->value.list;
    check(darray_end(l)==2, "Invalid extended response: odd number of elements in list.");
    tns_value_t *key=darray_get(l,0);
    check(key->type==tns_tag_string, "Invalid extended response: key is not a string");
    check(key->value.string != NULL,, "Invalid extended response: key is NULL");

    if(!bstrcmp(key->value.string, &XREQ_CTL)) {
        check (0 == handler_process_control_request(conn, data),
                "Control request processing returned non-zero: %s", bdata(key->value.string));
    } else {
        check (0 == dispatch_extended_request(conn, key->value.string, data),
                "Extended request dispatch returned non-zero: %s",bdata(key->value.string));
    }

    return;
error:
    tns_value_destroy(data);
    Register_disconnect(fd); // return ignored
    return;
}

static inline void handler_process_request(Handler *handler, int id, int fd,
        Connection *conn, bstring payload)
{
    int rc = 0;
    check(conn != NULL, "You can't pass NULL conn to this anymore.");

    if(blength(payload) == 0) {
        rc = Connection_deliver_raw(conn,NULL);
        check(rc != -1, "Register disconnect failed for: %d", fd);
    } else {
        int raw = conn->type != CONN_TYPE_MSG || handler->raw;

        rc = deliver_payload(raw, fd, conn, payload);
        check(rc != -1, "Failed to deliver to connection %d on socket %d", id, fd);
    }

    return;

error:
    Register_disconnect(fd);  // return ignored
    return;
}


static inline int handler_recv_parse(Handler *handler, HandlerParser *parser)
{
    zmq_msg_t *inmsg = NULL;
    check(handler->running, "Called while handler wasn't running, that's not good.");

    inmsg = calloc(sizeof(zmq_msg_t), 1);
    int rc = 0;

    check_mem(inmsg);

    rc = zmq_msg_init(inmsg);
    check(rc == 0, "Failed to initialize message.");

    taskstate("recv");

    rc = mqrecv(handler->recv_socket, inmsg, 0);
    check(rc == 0, "Receive on handler socket failed.");
    check(handler->running, "Handler marked as not running.");

    rc = HandlerParser_execute(parser, zmq_msg_data(inmsg), zmq_msg_size(inmsg));
    check(rc == 1, "Failed to parse message from handler.");

    check(parser->target_count > 0, "Message sent had 0 targets: %.*s",
            (int)zmq_msg_size(inmsg), (char *)zmq_msg_data(inmsg));

    zmq_msg_close(inmsg);
    free(inmsg);
    return 0;

error:
    if(inmsg) {
        zmq_msg_close(inmsg);
        free(inmsg);
    }
    return -1;
}


void Handler_task(void *v)
{
    int rc = 0;
    int i = 0;
    Handler *handler = (Handler *)v;
    HandlerParser *parser = NULL;
    int max_targets = Setting_get_int("limits.handler_targets", 128);
    log_info("MAX allowing limits.handler_targets=%d", max_targets);

    parser = HandlerParser_create(max_targets);
    check_mem(parser);

    check(Handler_setup(handler) == 0, "Failed to initialize handler, exiting.");

    while(handler->running && !task_was_signaled()) {
        taskstate("delivering");

        rc = handler_recv_parse(handler, parser);

        if(task_was_signaled()) {
            log_warn("Handler task signaled, exiting.");
            break;
        } else if( rc == -1 || parser->target_count <= 0) {
            log_warn("Skipped invalid message from handler: %s", bdata(handler->send_spec));
            taskdelay(100);
            continue;
        } else {
            for(i = 0; i < (int)parser->target_count; i++) {
                int id = (int)parser->targets[i];
                int fd = Register_fd_for_id(id);
                Connection *conn = fd == -1 ? NULL : Register_fd_exists(fd);

                // don't bother calling process request if there's nothing to handle
                if(conn && fd >= 0) {
                    if(parser->extended) {
                        handler_process_extended_request(fd, conn, parser->body);
                    } else {
                        handler_process_request(handler, id, fd, conn, parser->body);
                    }
                } else {
                    // TODO: I believe we need to notify the connection that it is dead too
                    Handler_notify_leave(handler, id);
                }
            }
        }

        HandlerParser_reset(parser);
    }

    handler->running = 0;
    handler->task = NULL;
    HandlerParser_destroy(parser);
    taskexit(0);

error:
    log_err("HANDLER TASK DIED: %s", bdata(handler->send_spec));
    handler->running = 0;
    handler->task = NULL;
    HandlerParser_destroy(parser);
    taskexit(1);
}


int Handler_deliver(void *handler_socket, char *buffer, size_t len)
{
    int rc = 0;
    zmq_msg_t msg;

    rc = zmq_msg_init(&msg);
    check(rc == 0, "Failed to initialize 0mq message to send.");

    rc = zmq_msg_init_data(&msg, buffer, len, cstr_free, NULL);
    check(rc == 0, "Failed to init 0mq message data.");

    rc = mqsend(handler_socket, &msg, 0);
    check(rc == 0, "Failed to deliver 0mq message to handler.");

    // zeromq owns the ram now
    return 0;

error:
    free(buffer); // we own the ram now
    return -1;
}


void *Handler_send_create(const char *send_spec, const char *identity)
{
    int bind_attempts = 10;
    void *handler_socket = mqsocket(ZMQ_PUSH);
    int rc = zmq_setsockopt(handler_socket, ZMQ_IDENTITY, identity, strlen(identity));
    check(rc == 0, "Failed to set handler socket %s identity %s", send_spec, identity);

    log_info("Binding handler PUSH socket %s with identity: %s", send_spec, identity);

    rc = zmq_bind(handler_socket, send_spec);

    while(rc != 0 && bind_attempts-- > 0) {
        taskdelay(1000);
        log_warn("Failed to bind send socket trying again for: %s", send_spec);
        rc = zmq_bind(handler_socket, send_spec);
    }

    check(bind_attempts > 0, "Too many bind attempts for handler %s", send_spec);

    return handler_socket;

error:
    return NULL;
}

void *Handler_recv_create(const char *recv_spec, const char *uuid)
{
    int bind_attempts = 10;
    void *listener_socket = mqsocket(ZMQ_SUB);
    check(listener_socket, "Can't create ZMQ_SUB socket.");

    int rc = zmq_setsockopt(listener_socket, ZMQ_SUBSCRIBE, uuid, strlen(uuid));
    check(rc == 0, "Failed to subscribe listener socket: %s", recv_spec);
    log_info("Binding listener SUB socket %s subscribed to: %s", recv_spec, uuid);

    rc = zmq_bind(listener_socket, recv_spec);

    while(rc != 0 && bind_attempts-- > 0) {
        taskdelay(1000);
        debug("Failed to bind recv socket trying again.");
        rc = zmq_bind(listener_socket, recv_spec);
    }

    check(bind_attempts > 0, "Too many bind attempts for handler %s", recv_spec);

    return listener_socket;

error:
    return NULL;
}

const int DEFAULT_HANDLER_STACK = 100 * 1024;

Handler *Handler_create(bstring send_spec, bstring send_ident,
        bstring recv_spec, bstring recv_ident)
{
    debug("Creating handler %s:%s", bdata(send_spec), bdata(send_ident));

    if(!HANDLER_STACK) {
        HANDLER_STACK = Setting_get_int("limits.handler_stack", DEFAULT_HANDLER_STACK);
        log_info("MAX limits.handler_stack=%d", HANDLER_STACK);
    }

    Handler *handler = calloc(sizeof(Handler), 1);
    check_mem(handler);

    handler->send_ident = bstrcpy(send_ident);
    handler->recv_ident = bstrcpy(recv_ident);
    handler->recv_spec = bstrcpy(recv_spec);
    handler->send_spec = bstrcpy(send_spec);
    handler->running = 0;
    handler->raw = 0;
    handler->protocol = HANDLER_PROTO_JSON;

    return handler;
error:

    if(handler) free(handler);
    return NULL;
}


void Handler_destroy(Handler *handler)
{
    if(handler) {
        if(handler->recv_socket) zmq_close(handler->recv_socket);
        if(handler->send_socket) zmq_close(handler->send_socket);

        bdestroy(handler->send_ident);
        bdestroy(handler->recv_ident);
        bdestroy(handler->send_spec);
        bdestroy(handler->recv_spec);
        free(handler);
    }
}
