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

#include "log.h"
#include "logrotate.h"
#include "dbg.h"
#include "request.h"
#include "headers.h"
#include "setting.h"
#include "tnetstrings.h"
#include <stdio.h>
#include "zmq_compat.h"
#include <pthread.h>

static void *LOG_SOCKET = NULL;
pthread_t LOG_THREAD;

typedef struct LogConfig {
    bstring file_name;
    bstring log_spec;
    FILE *log_file;
} LogConfig;


void LogConfig_destroy(LogConfig *config)
{
    if(config) {
        bdestroy(config->file_name);
        bdestroy(config->log_spec);
        if(config->log_file) fclose(config->log_file);
        free(config);
    }
}

static void *Log_internal_thread(void *spec)
{
    zmq_msg_t msg;
    int rc = 0;
    LogConfig *config = spec;

    void *socket = zmq_socket(ZMQ_CTX, ZMQ_SUB);
    check(socket, "Could not bind the logging subscribe socket.");

    // warning: could cause threading problems if more than one of these

    rc = zmq_setsockopt(socket, ZMQ_SUBSCRIBE, "", 0);
    check(rc == 0, "Could not subscribe to the logger.");

#ifdef ZMQ_LINGER
    int opt = 0;
    rc = zmq_setsockopt(socket, ZMQ_LINGER, &opt, sizeof(opt));
    check(rc == 0, "Could not set the linger option.");
#endif

    rc = zmq_connect(socket, bdata(config->log_spec));
    check(rc == 0, "Could connect to logging endpoint: %s", bdata(config->log_spec));


    while(1) {
       rc = zmq_msg_init(&msg);
       check(rc == 0, "Failed to initialize message.");

       rc = zmq_msg_recv(&msg, socket, 0);
       if(rc == -1 && errno == ETERM) {
           // The ZMQ context has been terminated, must be shutting down.
           break;
       }
       check(rc >= 0, "Failed to receive from the zeromq logging socket");
       check(zmq_msg_size(&msg) > 0, "Received poison pill, log thread exiting.");

       fprintf(config->log_file, "%.*s", (int)zmq_msg_size(&msg), (char *)zmq_msg_data(&msg));

       rc = zmq_msg_close(&msg);
       check(rc == 0, "Message close failed.");
    }

    rc = zmq_close(socket);
    check(rc == 0, "Could not close socket");

    LogConfig_destroy(config);
    return NULL;

error: 
    LogConfig_destroy(config);
    // could leak the socket and the msg but not much we can do
    return NULL;
}


LogConfig *LogConfig_create(bstring access_log, bstring log_spec)
{
    LogConfig *config = malloc(sizeof(LogConfig));
    config->log_spec = log_spec;
    config->file_name = access_log;

    config->log_file = fopen((char *)config->file_name->data, "a+");
    check(config->log_file, "Failed to open log file: %s for access logging.", bdata(config->file_name));
    setbuf(config->log_file, NULL);
    check(0==add_log_to_rotate_list(config->file_name,config->log_file), "Failed to add access log to list of logs to rotate.");

    return config;

error:
    LogConfig_destroy(config);
    return NULL;
}


int Log_init(bstring access_log, bstring log_spec)
{
    int rc = 0;
    LogConfig *config = NULL;

    if(LOG_SOCKET == NULL) 
    {
        check(ZMQ_CTX, "No ZMQ context, cannot start access log.");

        if(Setting_get_int("disable.access_logging", 0))
        {
            log_info("Access log is disabled according to disable.access_logging.");
        } 
        else 
        {
            config = LogConfig_create(access_log, log_spec);
            check(config, "Failed to configure access logging.");

            LOG_SOCKET = zmq_socket(ZMQ_CTX, ZMQ_PUB);
            check(LOG_SOCKET != NULL, "Failed to create access log socket");

#ifdef ZMQ_LINGER
            int opt = 0;
            rc = zmq_setsockopt(LOG_SOCKET, ZMQ_LINGER, &opt, sizeof(opt));
            check(rc == 0, "Could not set the linger option.");
#endif

            rc = zmq_bind(LOG_SOCKET, bdata(log_spec));
            check(rc == 0, "Failed to bind access_log zeromq socket.");

            pthread_create(&LOG_THREAD, NULL, Log_internal_thread, config);
        }
    }

    return 0;
error:

    LogConfig_destroy(config);
    return -1;
}


int Log_poison_workers()
{
    check(LOG_SOCKET != NULL, "No access log socket.");

    zmq_msg_t msg;
    int rc = zmq_msg_init_size(&msg, 0);
    check(rc == 0, "Could not create zmq message.");
    
    rc = zmq_msg_send(&msg, LOG_SOCKET, 0);
    check(rc >= 0, "Could not send message");

    rc = zmq_msg_close(&msg);
    check(rc == 0, "Failed to close message object");

    return 0;
error:
    zmq_msg_close(&msg);
    return -1;
}

static inline bstring make_log_message(Request *req, const char *remote_addr,
        int remote_port, int status, int size)
{
    bstring request_method = NULL;

    if (Request_is_json(req)) {
        request_method = &JSON_METHOD;
    } else if (Request_is_xml(req)) {
        request_method = &XML_METHOD;
    } else {
        request_method = req->request_method;
    }

    tns_outbuf outbuf = {.buffer = NULL};
    bstring b_temp;

    check(tns_render_log_start(&outbuf), "Could not initialize buffer");

    tns_render_number_prepend(&outbuf, size);
    tns_render_number_prepend(&outbuf, status);

    b_temp = bfromcstr(Request_is_json(req) ? "" : bdata(req->version));
    tns_render_string_prepend(&outbuf, b_temp);
    bdestroy(b_temp);

    tns_render_string_prepend(&outbuf, Request_path(req));
    tns_render_string_prepend(&outbuf, request_method);
    tns_render_number_prepend(&outbuf, (int) time(NULL));
    tns_render_number_prepend(&outbuf, remote_port);

    b_temp = bfromcstr(remote_addr);
    tns_render_string_prepend(&outbuf, b_temp);
    bdestroy(b_temp);

    tns_render_string_prepend(&outbuf, req->target_host->name);

    tns_render_log_end(&outbuf);

    // log_data now owns the outbuf buffer
    bstring log_data = tns_outbuf_to_bstring(&outbuf);
    bconchar(log_data, '\n');

    return log_data;

error:

    return NULL;
}

static void free_log_msg(void *data, void *hint)
{
    (void)data;

    bdestroy((bstring)hint);
}

int Log_request(Connection *conn, int status, int size)
{
    zmq_msg_t msg;

    if(LOG_SOCKET == NULL) 
        return 0;

    bstring log_data = make_log_message(conn->req, conn->remote, conn->rport, status, size);
    check_mem(log_data);

    int rc = zmq_msg_init_data(&msg, bdata(log_data), blength(log_data),
            free_log_msg, log_data);
    check(rc == 0, "Could not craft message for log message '%s'.", bdata(log_data));
    
    rc = zmq_msg_send(&msg, LOG_SOCKET, 0);
    check(rc >= 0, "Could not send log message to socket.");

    log_data = NULL; // that way errors from above can clean the log_data
    rc = zmq_msg_close(&msg);
    check(rc == 0, "Failed to close message object.");

    return 0;

error:
    bdestroy(log_data);
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
