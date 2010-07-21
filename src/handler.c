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
#include <task/task.h>
#include <zmq.h>
#include <dbg.h>
#include <stdlib.h>
#include <string.h>
#include <connection.h>
#include <assert.h>
#include <register.h>

struct tagbstring LEAVE_HEADER = bsStatic("{\"METHOD\":\"JSON\"}");
struct tagbstring LEAVE_MSG = bsStatic("{\"type\":\"disconnect\"}");

extern int RUNNING;


void bstring_free(void *data, void *hint)
{
    bdestroy((bstring)hint);
}


void Handler_notify_leave(Handler *handler, int fd)
{
    void *socket = handler->send_socket;
    assert(socket && "Socket can't be NULL");

    bstring payload = bformat("%s %d @* %d:%s,%d:%s,",
            bdata(handler->send_ident), fd,
            blength(&LEAVE_HEADER), bdata(&LEAVE_HEADER),
            blength(&LEAVE_MSG), bdata(&LEAVE_MSG));

    if(Handler_deliver(socket, bdata(payload), blength(payload)) == -1) {
        log_err("Can't tell handler %d died.", fd);
    }

    bdestroy(payload);
}

enum {
    MAX_TARGETS=100
};

struct target_splits {
    bstring data;
    int fds[MAX_TARGETS];
    int cur_fd;
    int last_pos;
};

int split_first_integers(void * param, int ofs, int len)
{
    struct target_splits *sp = (struct target_splits *)param;
    check(sp->cur_fd < MAX_TARGETS, "Too many targets given on send, max is %d", MAX_TARGETS);

    if(!isdigit(bchar(sp->data, ofs))) {
        sp->last_pos = ofs;
        return -1;
    } else {
        int fd = 0;
        char *endptr = NULL;
        char *s = bdataofs(sp->data, ofs);
        // TODO: it's parser time, this is getting ugly
        fd = (int)strtol(s, &endptr, 10);
        if(*endptr == ' ') {
            sp->fds[sp->cur_fd++] = fd;
        } else {
            sp->last_pos = ofs;
            return -1;
        }
    }

    return 0;
error:
    return -1;
}



void Handler_task(void *v)
{
    zmq_msg_t *inmsg = calloc(sizeof(zmq_msg_t), 1);
    struct target_splits splits;
    bstring data = bfromcstr("");
    int rc = 0;
    int i = 0;
    Handler *handler = (Handler *)v;
    bstring payload = NULL;

    taskname("Handler_task");

    handler->send_socket = Handler_send_create(bdata(handler->send_spec), bdata(handler->send_ident));
    check(handler->send_socket, "Failed to create handler socket.");

    handler->recv_socket = Handler_recv_create(bdata(handler->recv_spec), bdata(handler->recv_ident));
    check(handler->recv_socket, "Failed to create listener socket.");


    while(RUNNING) {
        zmq_msg_init(inmsg);

        taskstate("recv");
        rc = mqrecv(handler->recv_socket, inmsg, 0);
        check(rc == 0, "Receive on handler socket failed.");

        bassignblk(data, zmq_msg_data(inmsg), zmq_msg_size(inmsg));

        splits.cur_fd = 0;
        splits.last_pos = 0;
        splits.data = data;

        rc = bsplitcb(data, ' ', 0, split_first_integers, &splits);

        taskstate("delivering");
        
        for(i = 0; i < splits.cur_fd; i++) {
            int fd = splits.fds[i];
            int conn_type = Register_exists(fd);

            switch(conn_type) {
                case 0:
                    log_err("Ident %d is no longer connected.", fd);
                    Handler_notify_leave(handler, fd);
                    break;

                case CONN_TYPE_MSG:
                    payload = bTail(data, blength(data) - splits.last_pos);

                    // TODO: find out why this would be happening
                    if(payload->data[payload->slen - 1] == '\0') {
                        btrunc(payload, blength(payload)-1);
                    } else if(payload->data[payload->slen] != '\0') {
                        bconchar(payload, '\0');
                    }

                    if(Connection_deliver(fd, payload) == -1) {
                        log_err("Error sending to MSG listener %d, closing them.", fd);
                        Register_disconnect(fd);
                    }

                    bdestroy(payload);
                    break;

                case CONN_TYPE_HTTP:
                    payload = bTail(data, blength(data) - splits.last_pos);
                    if(Connection_deliver_raw(fd, payload) == -1) {
                        log_err("Error sending raw message to HTTP listener %d, closing them.", fd);
                        Register_disconnect(fd);
                    }
                    bdestroy(payload);
                    break;

                default:
                    log_err("Attempt to send to an invalid listener type.  Closing them.");
                    Register_disconnect(fd);
                    break;
            }
        }
    }

    debug("HANDLER EXITED.");
    return;

error:
    log_err("HANDLER TASK DIED");
    return;
}

int Handler_deliver(void *handler_socket, char *buffer, size_t len)
{
    int rc = 0;
    zmq_msg_t *msg = NULL;
    bstring msg_buf;

    msg = calloc(sizeof(zmq_msg_t), 1);
    msg_buf = NULL;

    check_mem(msg);

    rc = zmq_msg_init(msg);
    check(rc == 0, "Failed to initialize 0mq message to send.");

    msg_buf = blk2bstr(buffer, len);
    check_mem(msg_buf);

    rc = zmq_msg_init_data(msg, bdata(msg_buf), blength(msg_buf), bstring_free, msg_buf);
    check(rc == 0, "Failed to init 0mq message data.");

    rc = mqsend(handler_socket, msg, 0);
    check(rc == 0, "Failed to deliver 0mq message to handler.");

    if(msg) free(msg);
    return 0;

error:
    // TODO: confirm what if this is the right shutdown
    if(msg) free(msg);
    return -1;
}


void *Handler_send_create(const char *send_spec, const char *identity)
{
    
    void *handler_socket = mqsocket(ZMQ_DOWNSTREAM);
    int rc = zmq_setsockopt(handler_socket, ZMQ_IDENTITY, identity, strlen(identity));
    check(rc == 0, "Failed to set handler socket %s identity %s", send_spec, identity);

    log_info("Binding handler PUB socket %s with identity: %s", send_spec, identity);

    rc = zmq_bind(handler_socket, send_spec);
    check(rc == 0, "Can't bind handler socket: %s", send_spec);


    return handler_socket;

error:
    return NULL;
}

void *Handler_recv_create(const char *recv_spec, const char *uuid)
{
    void *listener_socket = mqsocket(ZMQ_SUB);
    check(listener_socket, "Can't create ZMQ_SUB socket.");

    int rc = zmq_setsockopt(listener_socket, ZMQ_SUBSCRIBE, uuid, 0);
    check(rc == 0, "Failed to subscribe listener socket: %s", recv_spec);
    log_info("Binding listener SUB socket %s subscribed to: %s", recv_spec, uuid);

    rc = zmq_bind(listener_socket, recv_spec);
    check(rc == 0, "Can't bind listener socket %s", recv_spec);

    return listener_socket;

error:
    return NULL;
}


Handler *Handler_create(const char *send_spec, const char *send_ident,
        const char *recv_spec, const char *recv_ident)
{
    Handler *handler = calloc(sizeof(Handler), 1);
    check_mem(handler);

    handler->send_ident = bfromcstr(send_ident);
    handler->recv_ident = bfromcstr(recv_ident);
    handler->recv_spec = bfromcstr(recv_spec);
    handler->send_spec = bfromcstr(send_spec);

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


