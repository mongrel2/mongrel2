#include <handler.h>
#include <task/task.h>
#include <zmq.h>
#include <dbg.h>
#include <stdlib.h>
#include <string.h>
#include <listener.h>
#include <assert.h>


static char *LEAVE_MSG = "{\"type\":\"leave\"}";
size_t LEAVE_MSG_LEN = 0;

void our_free(void *data, void *hint)
{
    free(data);
}


void Handler_init()
{
    LEAVE_MSG_LEN = strlen(LEAVE_MSG);
}


void Handler_notify_leave(void *socket, int fd)
{
    assert(socket && "Socket can't be NULL");

    if(Handler_deliver(socket, fd, LEAVE_MSG, LEAVE_MSG_LEN) == -1) {
        log_err("Can't tell handler %d died.", fd);
    }
}

void Handler_task(void *v)
{
    zmq_msg_t *inmsg = calloc(sizeof(zmq_msg_t), 1);
    char *data = NULL;
    size_t sz = 0;
    int fd = 0;
    int rc = 0;
    Handler *handler = (Handler *)v;

    while(1) {
        zmq_msg_init(inmsg);

        rc = mqrecv(handler->recv_socket, inmsg, 0);
        check(rc == 0, "Receive on handler socket failed.");

        data = (char *)zmq_msg_data(inmsg);
        sz = zmq_msg_size(inmsg);

        if(data[sz-1] != '\0') {
            log_err("Last char from handler is not 0 it's %d, fix your backend.", data[sz-1]);
        } if(data[sz-2] == '\0') {
            log_err("You have two \0 ending your message, fix your backend.");
        } else {
            int end = 0;
            int ok = sscanf(data, "%u%n", &fd, &end);
            debug("MESSAGE from handler: %s for fd: %d, nread: %d, len: %d, final: %d, last: %d",
                    data, fd, end, sz, sz-end-1, (data + end)[sz-end-1]);

            if(ok <= 0 || end <= 0) {
                log_err("Message didn't start with a ident number.");
            } else if(!Register_exists(fd)) {
                log_err("Ident %d is no longer connected.", fd);
                Handler_notify_leave(handler->send_socket, fd);
            } else {
                if(Listener_deliver(fd, data+end, sz-end-1) == -1) {
                    log_err("Error sending to listener %d, closing them.");
                    Register_disconnect(fd);
                    Handler_notify_leave(handler->send_socket, fd);
                }
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
    char *msg_buf = NULL;
    int msg_size = 0;
    size_t sz = 0;

    msg = calloc(sizeof(zmq_msg_t), 1);
    msg_buf = NULL;

    check(msg, "Failed to allocate 0mq message to send.");

    rc = zmq_msg_init(msg);
    check(rc == 0, "Failed to initialize 0mq message to send.");

    sz = strlen(buffer) + 32;
    msg_buf = malloc(sz);
    check(msg_buf, "Failed to allocate message buffer for handler delivery.");

    msg_size = snprintf(msg_buf, sz, "%d %.*s", from_fd, len, buffer);
    check(msg_size > 0, "Message too large, killing it.");

    rc = zmq_msg_init_data(msg, msg_buf, msg_size, our_free, NULL);
    check(rc == 0, "Failed to init 0mq message data.");

    rc = zmq_send(handler_socket, msg, 0);
    check(rc == 0, "Failed to deliver 0mq message to handler.");

    if(msg) free(msg);
    return 0;

error:
    if(msg) free(msg);
    if(msg_buf) free(msg_buf);
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
        free(handler);
    }
}
