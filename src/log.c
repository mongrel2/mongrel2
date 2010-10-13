#undef NDEBUG
#include "log.h"
#include "dbg.h"
#include "request.h"
#include "headers.h"
#include "setting.h"
#include <stdio.h>
#include <zmq.h>
#include <pthread.h>

static void *LOG_SOCKET = NULL;
pthread_t LOG_THREAD;

struct LoggingConfig {
    bstring file_name;
    bstring log_spec;
};

static void *Log_internal_thread(void *spec)
{
    zmq_msg_t msg;
    int rc = 0;
    FILE *log_file = NULL;
    struct LoggingConfig *config = spec;

    void *socket = zmq_socket(ZMQ_CTX, ZMQ_SUB);
    check(socket, "Could not bind the logging subscribe socket.");

    // warning: could cause threading problems if more than one of these

    rc = zmq_setsockopt(socket, ZMQ_SUBSCRIBE, "", 0);
    check(rc == 0, "Could not subscribe to the logger.");

    rc = zmq_connect(socket, bdata(config->log_spec));
    check(rc == 0, "Could connect to logging endpoint: %s", bdata(config->log_spec));

    log_file = fopen((char *)config->file_name->data, "a+");
    check(log_file, "Failed to open log file: %s for access logging.", bdata(config->file_name));
    setbuf(log_file, NULL);

    while(1) {
       rc = zmq_msg_init(&msg);
       check(rc == 0, "Failed to initialize message.");

       rc = zmq_recv(socket, &msg, 0);
       check(rc == 0, "Failed to receive from the zeromq logging socket.");
       check(zmq_msg_size(&msg) > 0, "Received poison pill, log thread exiting.");

       fprintf(log_file, "%.*s", (int)zmq_msg_size(&msg), (char *)zmq_msg_data(&msg));

       rc = zmq_msg_close(&msg);
       check(rc == 0, "Message close failed.");
    }

error: return NULL;
}

int Log_init(bstring access_log, bstring log_spec)
{
    int rc = 0;

    if(LOG_SOCKET == NULL) 
    {
        check(ZMQ_CTX, "No ZMQ context, cannot start access log.");

        if(Setting_get_int("disable.access_logging", 0))
        {
            log_info("Access log is disabled according to disable.access_logging.");
        } 
        else 
        {
            struct LoggingConfig *config = malloc(sizeof(struct LoggingConfig));
            config->log_spec = log_spec;
            config->file_name = access_log;

            LOG_SOCKET = zmq_socket(ZMQ_CTX, ZMQ_PUB);
            check(LOG_SOCKET != NULL, "Failed to create access log socket");

            rc = zmq_bind(LOG_SOCKET, bdata(log_spec));
            check(rc == 0, "Failed to bind access_log zeromq socket.");

            pthread_create(&LOG_THREAD, NULL, Log_internal_thread, config);
        }
    }

    return 0;
error:
    return -1;
}

int Log_poison_workers()
{
    check(LOG_SOCKET != NULL, "No access log socket.");

    zmq_msg_t msg;
    int rc = zmq_msg_init_size(&msg, 0);
    check(rc == 0, "Could not create zmq message.");
    
    rc = zmq_send(LOG_SOCKET, &msg, 0);
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

static inline bstring make_log_message(Request *req, const char *remote_addr, 
        int remote_port, int status, int size)
{
    bstring request_method = NULL;

    if(Request_is_json(req)) {
        request_method = &JSON_METHOD;
    } else if (Request_is_xml(req)) {
        request_method = &XML_METHOD;
    } else {
        request_method = req->request_method;
    }

    bstring log_data = bformat("%s,%.*s,%d,%d,%s,%s,%s,%d,%d\n",
            bdata(req->target_host->name),
            IPADDR_SIZE,
            remote_addr,
            remote_port,
            (int)time(NULL),
            request_method,
            bdata(Request_path(req)),
            Request_is_json(req) ? "" : bdata(req->version),
            status,
            size);

    return log_data;
}

int Log_request(Connection *conn, int status, int size)
{
    zmq_msg_t msg;

    if(LOG_SOCKET == NULL) 
        return 0;

    bstring log_data = make_log_message(conn->req, conn->remote, conn->rport, status, size);
    check_mem(log_data);

    int rc = zmq_msg_init_data(&msg, bdata(log_data), blength(log_data), free_log_msg, NULL);
    check(rc == 0, "Could not craft message for log message '%s'.", bdata(log_data));
    
    rc = zmq_send(LOG_SOCKET, &msg, 0);
    check(rc == 0, "Could not send log message to socket.");

    rc = zmq_msg_close(&msg);
    check(rc == 0, "Failed to close message object.");

    return 0;

error:
    zmq_msg_close(&msg);
    return -1;
}

int Log_term()
{
    if(LOG_SOCKET == NULL)
        return 0;

    int rc = zmq_close(LOG_SOCKET);
    check(rc == 0, "Failed to close access log socket.");

    return 0;

error:
    return -1;
}
