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

#include "control.h"
#include "bstring.h"
#include "task/task.h"
#include "register.h"
#include "server.h"
#include "dbg.h"
#include <stdlib.h>
#include <time.h>
#include "setting.h"
#include <signal.h>
#include "tnetstrings.h"
#include "tnetstrings_impl.h"
#include "version.h"


static void cstr_free(void *data, void *hint)
{
    (void)hint;

    free(data);
}

static int CONTROL_RUNNING = 1;
static void *CONTROL_SOCKET = NULL;

// I used python tnetstrings to print these out
struct tagbstring INVALID_FORMAT_ERR = bsStatic("101:4:code,15:INVALID_REQUEST,5:error,63:Invalid request format, must be a list with two elements in it.,}");
struct tagbstring INVALID_DATA_ERR = bsStatic("97:4:code,12:INVALID_DATA,5:error,62:Invalid data, expecting a list with a string and a hash in it.,}");
struct tagbstring CALLBACK_RETURN_ERR = bsStatic("91:4:code,15:CALLBACK_RETURN,5:error,53:Callback failed and returned NULL, should not happen.,}");
struct tagbstring CALLBACK_NOT_FOUND_ERR = bsStatic("74:4:code,18:CALLBACK_NOT_FOUND,5:error,33:Callback requested was not found.,}");
struct tagbstring SIGNAL_FAILED_ERR = bsStatic("75:4:code,13:SIGNAL_FAILED,5:error,39:raise call failed, can't signal process,}");
struct tagbstring INVALID_ARGUMENT_ERR = bsStatic("61:4:code,16:INVALID_ARGUMENT,5:error,22:Invalid argument type.,}");
struct tagbstring NOT_CONNECTED_ERR = bsStatic("57:4:code,13:NOT_CONNECTED,5:error,21:Socket not connected.,}");


#define check_control_err(A, E, M, ...) if(!(A)) { result = tns_parse(bdata(E), blength(E), NULL); log_warn(M, ##__VA_ARGS__); errno=0; goto error; }

typedef tns_value_t *(*callback_t)(bstring name, hash_t *args);

typedef struct callback_list_t {
    struct tagbstring name;
    struct tagbstring help;
    callback_t callback;
} callback_list_t;

callback_list_t CALLBACKS[];

static inline tns_value_t *read_message(void *sock)
{
    tns_value_t *req = NULL;
    int rc = 0;

    zmq_msg_t *inmsg = calloc(sizeof(zmq_msg_t), 1);
    rc = zmq_msg_init(inmsg);
    check(rc == 0, "init failed.");

    rc = mqrecv(sock, inmsg, 0);
    check(rc == 0, "Receive on control failed.");

    req = tns_parse(zmq_msg_data(inmsg), zmq_msg_size(inmsg), NULL);

    if(req == NULL) {
        req = tns_get_null();
    } 

    check(req, "Failed to create the request string.");

    zmq_msg_close(inmsg);
    free(inmsg);

    return req;

error:
    if(inmsg) {
        zmq_msg_close(inmsg);
        free(inmsg);
    }

    return NULL;
}


static inline int send_reply(void *sock, tns_value_t *rep)
{
    int rc = 0;
    size_t len = 0;

    zmq_msg_t *outmsg = calloc(sizeof(zmq_msg_t), 1);
    check_mem(outmsg);

    rc = zmq_msg_init(outmsg);
    check(rc == 0, "Cannot allocate zmq msg.");

    char *outbuf = tns_render(rep, &len);
    check(outbuf != NULL, "Failed to render control port reply.");
    check(len > 0, "Control port reply is too small.");

    rc = zmq_msg_init_data(outmsg, outbuf, len, cstr_free, rep);
    check(rc == 0, "Failed to init reply data.");
    
    rc = mqsend(sock, outmsg, 0);
    check(rc == 0, "Failed to deliver 0mq message to requestor.");
    free(outmsg);
    return 0;

error:
    free(outmsg);
    return -1;
}


/**
 * Does the most common response you'll make, which is a simple dict
 * with a single key=value.  It takes over ownership of the key and
 * value you pass in, so bstrcpy it if you don't want it to be 
 * destroyed.
 */
static inline tns_value_t *basic_response(bstring key, bstring value)
{
    tns_value_t *results = tns_new_dict();
    tns_value_t *headers = tns_new_list();
    tns_value_t *rows = tns_new_list();
    tns_value_t *cols = tns_new_list();

    tns_add_to_list(headers, tns_parse_string(bdata(key), blength(key)));
    tns_add_to_list(cols, tns_parse_string(bdata(value), blength(value)));
    tns_add_to_list(rows, cols);

    bdestroy(key);
    bdestroy(value);

    tns_dict_setcstr(results, "headers", headers);
    tns_dict_setcstr(results, "rows", rows);

    return results;
}


static inline tns_value_t *get_arg(hash_t *args, const char *name)
{
    bstring key = bfromcstr(name);
    hnode_t *node = hash_lookup(args, key);
    bdestroy(key);

    check(node != NULL, "Missing argument %s in call.", name);

    tns_value_t *val = hnode_get(node);
    check(val != NULL, "Got a null for a dict in arguments.");

    return val;
error:
    return NULL;
}

static tns_value_t *signal_server_cb(bstring name, hash_t *args) 
{
    (void)args;

    int rc = -1;
    tns_value_t *result = NULL;

    if(biseqcstr(name, "stop")) {
        rc = raise(SIGINT);
    } else if(biseqcstr(name, "reload")) {
        rc = raise(SIGHUP);
    } else if(biseqcstr(name, "terminate")) {
        rc = raise(SIGTERM);
    } else {
        rc = -1;
    }

    check_control_err(rc == 0, &SIGNAL_FAILED_ERR, "Signal failed.");

    result = basic_response(bfromcstr("msg"), bfromcstr("signal sent to server"));

error: // fallthrough
    return result;
}


tns_value_t *version_cb(bstring name, hash_t *args)
{
    (void)name;
    (void)args;

    return basic_response(bfromcstr("version"), bfromcstr(VERSION));
}

tns_value_t *help_cb(bstring name, hash_t *args)
{
    (void)name;
    (void)args;

    tns_value_t *result = tns_new_dict();
    tns_value_t *headers = tns_new_list();
    tns_value_t *rows = tns_new_list();

    tns_add_to_list(headers, tns_parse_string("name", strlen("name")));
    tns_add_to_list(headers, tns_parse_string("help", strlen("help")));
    tns_dict_setcstr(result, "headers", headers);

    callback_list_t *cbel = NULL;

    for(cbel = CALLBACKS; cbel->callback != NULL; cbel++) {
        tns_value_t *col = tns_new_list();
        tns_add_to_list(col, tns_parse_string(bdata(&cbel->name), blength(&cbel->name)));
        tns_add_to_list(col, tns_parse_string(bdata(&cbel->help), blength(&cbel->help)));
        tns_add_to_list(rows, col);
    }

    tns_dict_setcstr(result, "rows", rows);

    return result;
}

tns_value_t *control_stop_cb(bstring name, hash_t *args)
{
    (void)name;
    (void)args;

    CONTROL_RUNNING = 0;

    return basic_response(bfromcstr("msg"), bfromcstr("stopping the control port"));
}

tns_value_t *kill_cb(bstring name, hash_t *args)
{
    (void)name;

    tns_value_t *result = tns_new_dict();

    tns_value_t *arg = get_arg(args, "id");
    check_control_err(arg != NULL, &INVALID_ARGUMENT_ERR, "Missing argument 'id'.");
    check_control_err(tns_get_type(arg) == tns_tag_number, &INVALID_ARGUMENT_ERR, "Argument type error.");

    int id = arg->value.number;
    check_control_err(id >= 0 && id < MAX_REGISTERED_FDS, &INVALID_ARGUMENT_ERR, "Argument type error.");


    int fd = Register_fd_for_id(id);
    check_control_err(fd >= 0, &INVALID_ARGUMENT_ERR, "Argument type error.");
    check_control_err(Register_fd_exists(fd), &NOT_CONNECTED_ERR, "Socket not connected.");

    Register_disconnect(fd);

    tns_value_destroy(result); // easier to just delete it and return a basic one

    return basic_response(bfromcstr("status"), bfromcstr("OK"));

error: // fallthrough
    return result;
}


tns_value_t *status_cb(bstring name, hash_t *args)
{
    (void)name;

    tns_value_t *val = get_arg(args, "what");
    tns_value_t *result = tns_new_dict();

    check_control_err(val != NULL, &INVALID_ARGUMENT_ERR, "Missing argument what.");
    check_control_err(val->type == tns_tag_string, &INVALID_ARGUMENT_ERR, "Argument type error.");
    bstring what = val->value.string;

    if(biseqcstr(what, "tasks")) {
        tns_value_destroy(result);
        return taskgetinfo();
    } else if(biseqcstr(what, "net")) {
        tns_value_destroy(result);
        return Register_info();
    } else {
        bstring err = bfromcstr("Expected argument what=['net'|'tasks'].");
        tns_dict_setcstr(result, "error", tns_parse_string(bdata(err), blength(err)));
        bdestroy(err);
    }

error: // fallthrough
    return result;
}

struct tagbstring INFO_HEADERS = bsStatic("92:4:port,9:bind_addr,4:uuid,6:chroot,10:access_log,9:error_log,8:pid_file,16:default_hostname,]");

tns_value_t *info_cb(bstring name, hash_t *args)
{
    (void)args;

    Server *srv = Server_queue_latest();

    if(biseqcstr(name, "time")) {
        return basic_response(bfromcstr("time"), bformat("%d", (int)time(NULL)));
    } else if(biseqcstr(name, "uuid")) {
        return basic_response(bfromcstr("uuid"), bstrcpy(srv->uuid));
    } else {
        tns_value_t *rows = tns_new_list();
        tns_value_t *cols = tns_new_list();
        tns_add_to_list(cols, tns_new_integer(srv->port));
        tns_list_addstr(cols, srv->bind_addr);
        tns_list_addstr(cols, srv->uuid);
        if(srv->chroot != NULL) {
            tns_list_addstr(cols, srv->chroot);
        } else {
            bstring empty = bfromcstr("");
            tns_list_addstr(cols, empty);
            bdestroy(empty);
        }
        tns_list_addstr(cols, srv->access_log);
        tns_list_addstr(cols, srv->error_log);
        tns_list_addstr(cols, srv->pid_file);
        tns_list_addstr(cols, srv->default_hostname);

        tns_add_to_list(rows, cols);
        return tns_standard_table(&INFO_HEADERS, rows);
    }
}


callback_list_t CALLBACKS[] = {
    {.name = bsStatic("stop"),
        .help = bsStatic("stop the server (SIGINT)"), .callback = signal_server_cb},
    {.name = bsStatic("reload"),
        .help = bsStatic("reload the server"), .callback = signal_server_cb},
    {.name = bsStatic("help"),
        .help = bsStatic("this command"), .callback = help_cb},
    {.name = bsStatic("control_stop"),
        .help = bsStatic("stop control port"), .callback = control_stop_cb},
    {.name = bsStatic("kill"),
        .help = bsStatic("kill a connection"), .callback = kill_cb},
    {.name = bsStatic("status"),
        .help = bsStatic("status, what=['net'|'tasks']"), .callback = status_cb},
    {.name = bsStatic("terminate"),
        .help = bsStatic("terminate the server (SIGTERM)"), .callback = signal_server_cb},
    {.name = bsStatic("time"),
        .help = bsStatic("the server's time"), .callback = info_cb},
    {.name = bsStatic("uuid"),
        .help = bsStatic("the server's uuid"), .callback = info_cb},
    {.name = bsStatic("info"),
        .help = bsStatic("information about this server"), .callback = info_cb},

    {.name = bsStatic(""), .help = bsStatic(""), .callback = NULL},
};


tns_value_t *Control_execute(tns_value_t *req, callback_list_t *callbacks)
{
    tns_value_t *result = NULL;
    tns_value_t *call = NULL;
    tns_value_t *args = NULL;
    callback_list_t *cbel = NULL;

    // validate all the possible things that can get screwed up in a request
    check_control_err(req->type == tns_tag_list, &INVALID_FORMAT_ERR, "Invalid request format.");

    call = darray_get(req->value.list, 0);
    check_control_err(call != NULL, &INVALID_DATA_ERR, "Invalid first argument.");
    check_control_err(call->type == tns_tag_string, &INVALID_DATA_ERR, "First argument must be string.");

    // validate second argument is a dict
    args = darray_get(req->value.list, 1);
    check_control_err(args != NULL, &INVALID_DATA_ERR, "Invalid second argument.");
    check_control_err(args != call, &INVALID_DATA_ERR, "Must have more than one element to request.");
    check_control_err(args->type == tns_tag_dict, &INVALID_DATA_ERR, "Second argument must be dict.");

    // TODO: the above code will actually ignore anything but the first and last argument
    // which should make it more robust, but might want to tighten

    // find the matching callback
    for(cbel = callbacks; cbel->callback != NULL; cbel++) {
        if(biseq(call->value.string, &cbel->name)) {
            result = cbel->callback(call->value.string, args->value.dict);
            check_control_err(result != NULL, &CALLBACK_RETURN_ERR, "Callback return invalid data (NULL).");
            break;
        }
    }

    // if result is still NULL then it's an error we didn't find anything
    check_control_err(result != NULL, &CALLBACK_NOT_FOUND_ERR, "Callback not found.");

error: // fallthrough on purpose
    return result;
}

struct tagbstring DEFAULT_CONTROL_SPEC = bsStatic("ipc://run/control");

void Control_task(void *v)
{
    (void)v;

    int rc = 0;
    tns_value_t *req = NULL;
    tns_value_t *rep = NULL;

    bstring spec;
    Server *srv = Server_queue_latest();
    if(srv->control_port != NULL) {
        spec = srv->control_port;
    } else {
        spec = Setting_get_str("control_port", &DEFAULT_CONTROL_SPEC);
    }

    taskname("control");

    log_info("Setting up control socket in at %s", bdata(spec));

    CONTROL_SOCKET = mqsocket(ZMQ_REP);
    check(CONTROL_SOCKET != NULL, "Can't create contol socket.");

    rc = zmq_bind(CONTROL_SOCKET, bdata(spec));
    check(rc == 0, "Failed to bind control port to run/control.");

    while(CONTROL_RUNNING) {
        taskstate("waiting");

        req = read_message(CONTROL_SOCKET);
        check(req, "Failed to read message: %s.", strerror(errno));

        if(req->type == tns_tag_null) {
            rep = tns_parse(bdata(&INVALID_DATA_ERR), blength(&INVALID_DATA_ERR), NULL);
        } else {
            rep = Control_execute(req, CALLBACKS);
        }

        check(rep != NULL, "Should never get a NULL from the control port executor.");

        taskstate("replying");
        rc = send_reply(CONTROL_SOCKET, rep);
        check(rc == 0, "Failed to send a reply.");

        tns_value_destroy(req);
        tns_value_destroy(rep);
    }

    rc = zmq_close(CONTROL_SOCKET);
    check(rc == 0, "Failed to close control port socket.");
    CONTROL_SOCKET = NULL;
    log_info("Control port exiting.");
    taskexit(0);

error:
    log_info("Control port exiting with error.");
    zmq_close(CONTROL_SOCKET);
    CONTROL_SOCKET = NULL;
    taskexit(1);
}


void Control_port_start()
{
    taskcreate(Control_task, NULL, 32 * 1024);
}

void Control_port_stop()
{
    CONTROL_RUNNING = 0;
}
