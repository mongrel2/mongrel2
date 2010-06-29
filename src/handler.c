#include <handler.h>
#include <task/task.h>
#include <zmq.h>
#include <dbg.h>
#include <stdlib.h>
#include <string.h>
#include <listener.h>
#include <assert.h>
#include <register.h>


struct tagbstring LEAVE_MSG = bsStatic("@* {\"type\":\"leave\"}");

void bstring_free(void *data, void *hint)
{
    bdestroy((bstring)hint);
}


void Handler_notify_leave(void *socket, int fd)
{
    assert(socket && "Socket can't be NULL");

    if(Handler_deliver(socket, fd, bdata(&LEAVE_MSG), blength(&LEAVE_MSG)) == -1) {
        log_err("Can't tell handler %d died.", fd);
    }
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
        char *s = bdataofs(sp->data, ofs);
        sp->fds[sp->cur_fd++] = atoi(s);
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

    taskname("Handler_task");

    while(1) {
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

            if(!Register_exists(fd)) {
                log_err("Ident %d is no longer connected.", fd);
                Handler_notify_leave(handler->send_socket, fd);
            } else {
                bstring payload = bTail(data, blength(data) - splits.last_pos);

                // TODO: find out why this would be happening
                if(payload->data[payload->slen - 1] == '\0') {
                    btrunc(payload, blength(payload)-1);
                } else if(payload->data[payload->slen] != '\0') {
                    bconchar(payload, '\0');
                }

                if(Listener_deliver(fd, payload) == -1) {
                    log_err("Error sending to listener %d, closing them.", fd);
                    Register_disconnect(fd);
                    Handler_notify_leave(handler->send_socket, fd);
                }

                bdestroy(payload);
            }
        }
    }

    return;
error:
    taskexitall(1);
}

int Handler_deliver(void *handler_socket, int from_fd, char *buffer, size_t len)
{
    int rc = 0;
    zmq_msg_t *msg = NULL;
    bstring msg_buf;

    msg = calloc(sizeof(zmq_msg_t), 1);
    msg_buf = NULL;

    check(msg, "Failed to allocate 0mq message to send.");

    rc = zmq_msg_init(msg);
    check(rc == 0, "Failed to initialize 0mq message to send.");

    msg_buf = bformat("%d %.*s", from_fd, len, buffer);
    check(msg_buf, "Failed to allocate message buffer for handler delivery.");

    rc = zmq_msg_init_data(msg, bdata(msg_buf), blength(msg_buf), bstring_free, msg_buf);
    check(rc == 0, "Failed to init 0mq message data.");

    rc = zmq_send(handler_socket, msg, 0);
    check(rc == 0, "Failed to deliver 0mq message to handler.");

    if(msg) free(msg);
    return 0;

error:
    // TODO: confirm what if this is the right shutdown
    if(msg) free(msg);
    if(msg_buf) bdestroy(msg_buf);
    return -1;
}


void *Handler_send_create(const char *send_spec, const char *identity)
{
    
    void *handler_socket = mqsocket(ZMQ_PUB);
    int rc = zmq_setsockopt(handler_socket, ZMQ_IDENTITY, identity, strlen(identity));
    check(rc == 0, "Failed to set handler socket %s identity %s", send_spec, identity);

    debug("Binding handler PUB socket %s with identity: %s", send_spec, identity);

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
    debug("binding listener SUB socket %s subscribed to: %s", recv_spec, uuid);

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
    check(handler, "Memory allocate failed.");

    handler->send_socket = Handler_send_create(send_spec, send_ident);
    check(handler->send_socket, "Failed to create handler socket.");

    handler->recv_socket = Handler_recv_create(recv_spec, recv_ident);
    check(handler->recv_socket, "Failed to create listener socket.");

    return handler;
error:

    if(handler) free(handler);
    return NULL;
}


void Handler_destroy(Handler *handler, int fd)
{
    if(handler) {
        if(fd) {
            Handler_notify_leave(handler->send_socket, fd);
        }

        zmq_close(handler->recv_socket);
        zmq_close(handler->send_socket);

        free(handler);
    }
}
