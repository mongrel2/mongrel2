#include "log.h"

#include <stdio.h>

#include <zmq.h>

#include "dbg.h"
#include "request.h"
#include "headers.h"
#include "setting.h"

static void *access_socket = NULL;

int Log_init()
{
    if(access_socket == NULL) 
    {
        check(ZMQ_CTX, "No ZMQ context, cannot start access log.");

        if(Setting_get_int("disable.access_logging", 0))
        {
            log_info("Access log is disabled according to disable.access_logging.");
        } 
        else 
        {
            access_socket = zmq_socket(ZMQ_CTX, ZMQ_PUB); //fopen(bdata(server->access_log), "a+");
            check(access_socket != NULL, "Failed to create access log socket");
        }
    }

    return 0;
error:
    return -1;
}

int Log_bind(const char *endpoint)
{
    check(access_socket != NULL, "No access log socket to bind to.");

    int rc = zmq_bind(access_socket, endpoint);
    check(rc == 0, "Could not bind to endpoint %s.", endpoint);

    return 0;
error:
    return -1;
}

int Log_poison_workers()
{
    check(access_socket != NULL, "No access log socket.");

    zmq_msg_t msg;
    int rc = zmq_msg_init_size(&msg, 0);
    check(rc == 0, "Could not create zmq message.");
    
    rc = zmq_send(access_socket, &msg, 0);
    check(rc == 0, "Could not send message");

    rc = zmq_msg_close(&msg);
    check(rc == 0, "Failed to close message object");

    return 0;
error:
    zmq_msg_close(&msg);
    return -1;
}

static void free_log_msg(void *data, void *hint)
{
    bdestroy(data);
}

int Log_request(Connection *conn, int status, int size)
{
    if(access_socket == NULL) 
        return 0;

    Request *req = conn->req;

    // TODO: there's a security hole here in that people can inject xterm codes from this
    bstring log_data = bformat("%s %.*s -- -- %d \"%s %s %s\" %d %d\n",
            bdata(req->target_host->name),
            IPADDR_SIZE,
            conn->remote,
            (int)time(NULL),
            Request_is_json(req) ? "JSON" : bdata(req->request_method),
            Request_is_json(req) ? bdata(Request_path(req)) : bdata(req->uri),
            Request_is_json(req) ? "" : bdata(req->version),
            status,
            size);
    check_mem(log_data);

    zmq_msg_t msg;
    int rc = zmq_msg_init_data(&msg, bdata(log_data), blength(log_data), free_log_msg, NULL);
    check(rc == 0, "Could not craft message for log message '%s'.", bdata(log_data));
    
    rc = zmq_send(access_socket, &msg, 0);
    check(rc == 0, "Could not send log message to socket.");

    rc = zmq_msg_close(&msg);
    check(rc == 0, "Failed to close message object.");

    bdestroy(log_data);

    return 0;

error:
    zmq_msg_close(&msg);
    bdestroy(log_data);
    return -1;
}

int Log_term()
{
    if(access_socket == NULL)
        return 0;

    int rc = zmq_close(access_socket);
    check(rc == 0, "Failed to close access log socket.");

    return 0;

error:
    return -1;
}
