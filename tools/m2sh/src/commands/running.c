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

#include <signal.h>

#include <zmq.h>

#include <tnetstrings.h>
#include <tnetstrings_impl.h>
#include <config/db.h>
#include <handler.h>
#include <pattern.h>
#include <register.h>

#include "../linenoise.h"
#include "../commands.h"
#include "../query_print.h"
#include "logging.h"
#include "running.h"

struct ServerRun {
    int ran;
    bstring db_file;
    bstring config_url;
    bstring config_style;
    bstring uuid;
    const char *sudo;
    int murder;
};

static inline int exec_server_operations(Command *cmd,
        int (*callback)(struct ServerRun *, tns_value_t *), const char *select)
{
    int rc = 0;
    tns_value_t *res = NULL;

    bstring db_file = option(cmd, "db", NULL);
    bstring conf_url = option(cmd, "url", NULL);
    bstring conf_style = option(cmd, "style", NULL);
    bstring uuid = option(cmd, "uuid", NULL);

    if(conf_url == NULL && db_file == NULL) {
        db_file = bfromcstr("config.sqlite");
    }

    check(db_file != NULL || (conf_url != NULL && conf_style != NULL),
            "You must give either --db or --style and --url.");

    struct ServerRun run = {
        .ran = 0,
        .db_file = db_file,
        .config_url = conf_url,
        .config_style = conf_style,
        .sudo = "",
        .uuid = uuid,
        .murder = 0
    };

    bstring name = option(cmd, "name", NULL);
    bstring host = option(cmd, "host", NULL);
    bstring sudo = option(cmd, "sudo", NULL);
    bstring every = option(cmd, "every", NULL);
    run.murder = option(cmd, "murder", NULL) != NULL;

    check(!(name && uuid && host), "Just one please, not all of the options.");

    if(sudo) {
        run.sudo = biseqcstr(sudo, "") ? "sudo" : bdata(sudo);
    } else {
        run.sudo = "";
    }

    if(uuid == NULL) {
        check(db_file != NULL, "There is no uuid, and no database given, need a uuid.");

        rc = DB_init(bdata(db_file));
        check(rc == 0, "Failed to open db: %s", bdata(db_file));

        if(every) {
            res = DB_exec("SELECT %s FROM server", select);
        } else if(name) {
            res = DB_exec("SELECT %s FROM server where name = %Q", select, bdata(name));
        } else if(host) {
            res = DB_exec("SELECT %s FROM server where default_host = %Q", select, bdata(host));
        } else if(uuid) {
            // yes, this is necessary, so that the callback runs
            res = DB_exec("SELECT %s FROM server where uuid = %Q", select, bdata(uuid));
        } else {
            sentinel("You must give either -name, -uuid, or -host to start a server.");
        }

        check(tns_get_type(res) == tns_tag_list,
                "Wrong return type from query, should be list.");
    }

    check(callback(&run, res) != -1, "Failed to run internal operation.");

    if(!run.ran) {
        errno = 0;

        if(every) {
            log_err("You specified -every server but didn't load any. Not configured right?");
        } else if(host) {
            log_err("Could not load server with host '%s' from db %s.", bdata(host), bdata(db_file));
        } else if(uuid) {
            log_err("Could not load server with uuid '%s' from db %s.", bdata(uuid), bdata(db_file));
        } else if(name) {
            log_err("Could not load server named '%s' from db %s.", bdata(name), bdata(db_file));
        } else {
            log_err("Well looks like you broke something, please report what you did to mongrel2.org.");
        }

        sentinel("Error loading the requested server, see above for why.");
    }

    if(res) tns_value_destroy(res);
    DB_close();
    return 0;

error:
    if(res) tns_value_destroy(res);
    DB_close();
    return -1;
}

static int run_server(struct ServerRun *r, tns_value_t *res)
{
    r->ran = 0;
    bstring config = NULL;
    bstring module = NULL;
    bstring uuid = NULL;

    if(r->db_file) {
        DB_check(res, 0, 1, tns_tag_string);
        tns_value_t *uuid_val = DB_get(res, 0, 0);

        config = bstrcpy(r->db_file);
        module = bfromcstr("");
        uuid = bstrcpy(uuid_val->value.string);
    } else {
        config = bstrcpy(r->config_url);
        module = bstrcpy(r->config_style);
        uuid = bstrcpy(r->uuid);
    }

    bstring command = bformat("%s mongrel2 %s %s %s",
            r->sudo, bdata(config), bdata(uuid), bdata(module));

    system(bdata(command));

    bdestroy(command);
    bdestroy(config);
    bdestroy(module);

    r->ran = 1;
    return 0;

error:
    return -1;
}

int Command_start(Command *cmd)
{
    return exec_server_operations(cmd, run_server, "uuid");
}

bstring read_pid_file(bstring pid_path)
{
    FILE *pid_file = fopen(bdata(pid_path), "r");
    bstring pid = NULL;

    if(pid_file == NULL) {
        return NULL;
    } else {
        pid = bread((bNread)fread, pid_file);
        fclose(pid_file); pid_file = NULL;
    }

    return pid;
}

static int locate_pid_file(tns_value_t *res)
{
    bstring pid = NULL;
    bstring pid_path = NULL;

    int cols = 0;
    int rows = DB_counts(res, &cols);
    check(rows == 1 && cols == 2, "Wrong number of results.");

    tns_value_t *chroot = DB_get(res, 0, 0);
    check(tns_get_type(chroot) == tns_tag_string, "Wrong result for server chroot, should be a string.");

    tns_value_t *pid_file = DB_get(res, 0, 1);
    check(tns_get_type(pid_file) == tns_tag_string, "Wrong result for server pid_file, should be a string.");

    pid_path = bformat("%s%s", bdata(chroot->value.string), bdata(pid_file->value.string));

    pid = read_pid_file(pid_path);
    check(pid, "Couldn't read the PID from %s", bdata(pid_path));

    int result = atoi((const char *)pid->data);

    bdestroy(pid);
    bdestroy(pid_path);
    return result;

error:
    bdestroy(pid);
    bdestroy(pid_path);
    return -1;
}

static int kill_server(struct ServerRun *r, tns_value_t *res, int signal)
{
    int pid = locate_pid_file(res);
    check(pid != -1, "Failed to read the pid_file.");

    int rc = kill(pid, signal);
    check(rc == 0, "Failed to stop server with PID: %d", pid);

    r->ran = 1;
    return 0;

error:
    r->ran = 0;
    return -1;
}


static int stop_server(struct ServerRun *r, tns_value_t *res)
{
    int signal = r->murder ? SIGTERM : SIGINT;
    return kill_server(r, res, signal);
}

int Command_stop(Command *cmd)
{
    return exec_server_operations(cmd, stop_server, "chroot, pid_file");
}

static int reload_server(struct ServerRun *r, tns_value_t *res)
{
    return kill_server(r, res, SIGHUP);
}

int Command_reload(Command *cmd)
{
    return exec_server_operations(cmd, reload_server, "chroot, pid_file");
}

static int check_server(struct ServerRun *r, tns_value_t *res)
{
    int rc = 0;
    int pid = locate_pid_file(res);

    if(pid == -1) {
        printf("mongrel2 is not running because pid_file isn't there.\n");
        r->ran = 1;
        return 0;
    }

    errno = 0;
    rc = kill(pid, 0);

    if((rc != 0) && (errno == ESRCH)) {
        printf("mongrel2 at PID %d is NOT running.\n", pid);
    } else if ((rc == 0) || (errno == EPERM)) {
        printf("mongrel2 at PID %d running.\n", pid);
    } else {
        sentinel("Could not send signal to mongrel2 at PID %d", pid);
    }

    r->ran = 1;
    return 0;

error:
    r->ran = 0;
    return -1;
}

int Command_running(Command *cmd)
{
    return exec_server_operations(cmd, check_server, "chroot, pid_file");
}

static inline int linenoise_runner(const char *prompt, int (*callback)(bstring arg, void *data), void *data)
{
    char *line = NULL;
    bstring args = NULL;
    char *home_dir = getenv("HOME");
    bstring hist_file = NULL;

    if(home_dir != NULL) {
        hist_file = bformat("%s/.m2sh", home_dir);
        linenoiseHistoryLoad(bdata(hist_file));
    } else {
        log_warn("You don't have a HOME environment variable. Oh well, no history.");
        hist_file = NULL;
    }

    while((line = linenoise(prompt)) != NULL) {
        if (line[0] != '\0') {
            args = bformat("%s", line);
            callback(args, data);
            bdestroy(args);

            if(hist_file) {
                linenoiseHistoryAdd(line);
                linenoiseHistorySave(bdata(hist_file)); /* Save every new entry */
            }
        }

        free(line);
    }

    bdestroy(hist_file);
    return 0;
}

static struct tagbstring TOKENS = bsStatic(" \t\n=");
static struct tagbstring NUMBER_PATTERN = bsStatic("[\\-0-9]+");
bstring parse_input(bstring inbuf)
{
    size_t len = 0;
    int i = 0;
    char *data = NULL;
    bstring result = NULL;
    tns_value_t *req = tns_new_list();
    tns_value_t *args = tns_new_dict();
    tns_value_t *value = NULL;

    btrimws(inbuf);
    struct bstrList *list = bsplits(inbuf, &TOKENS);
    check((list->qty + 1) % 2 == 0, "USAGE: command arg1=val1 arg2=val2");

    tns_add_to_list(req,
            tns_parse_string(bdata(list->entry[0]), blength(list->entry[0])));

    for(i = 1; i < list->qty; i += 2) {
        bstring key_str = list->entry[i];
        bstring val_str = list->entry[i+1];
        tns_value_t *key = tns_parse_string(bdata(key_str), blength(key_str));

        if(bstring_match(val_str, &NUMBER_PATTERN)) {
            value = tns_parse_integer(bdata(val_str), blength(val_str));
        } else {
            value = tns_parse_string(bdata(val_str), blength(val_str));
        }

        tns_add_to_dict(args, key, value);
    }

    tns_add_to_list(req, args);

    data = tns_render(req, &len);
    check(data != NULL, "Didn't render to a valid TNetstring.");
    check(len > 0, "Didn't create a valid TNetstring.");

    result = blk2bstr(data, len);
    check(result != NULL, "Couldn't convert to string.");

    tns_value_destroy(req);
    bstrListDestroy(list);
    free(data);
    return result;

error:

    if(req) tns_value_destroy(req);
    if(list) bstrListDestroy(list);
    if(data) free(data);
    if(result) bdestroy(result);

    return NULL;
}

static void bstring_free(void *data, void *hint)
{
    bdestroy((bstring)hint);
}

struct tagbstring ERROR_KEY = bsStatic("error");
struct tagbstring HEADERS_KEY = bsStatic("headers");
struct tagbstring ROWS_KEY = bsStatic("rows");

static void display_map_style(tns_value_t *headers, tns_value_t *table)
{
    int i = 0;
    int cols = 0;
    int rows = DB_counts(table, &cols);
    check(rows != -1, "Invalid query result, probably not a table.");
    darray_t *names = headers->value.list;

    check(cols == darray_end(names),
            "Server returned a bad result, names isn't same length as elements.");

    if(rows == 1) {
        for(i = 0; i < cols; i++)
        {
            tns_value_t *h = darray_get(names, i);
            tns_value_t *val = DB_get(table, 0, i);
            check(tns_get_type(h) == tns_tag_string, "Headers should be strings.");
            check(tns_get_type(val) != tns_tag_invalid,
                    "Invalid value for column %d of result.", i);

            print_datum(h);
            printf(":  ");
            print_datum(val);
            printf("\n");
        }
    } else {
        sentinel("Asked to display something that's not in map style.");
    }

error: // fallthrough
    return;
}


void display_table_style(tns_value_t *headers, tns_value_t *table)
{
    int col_i = 0;
    int row_i = 0;
    for(col_i = 0; col_i < darray_end(headers->value.list); col_i++)
    {
        tns_value_t *h = darray_get(headers->value.list, col_i);
        check(tns_get_type(h) == tns_tag_string,
                "Headers should be strings, not: %c", tns_get_type(h));
        printf("%s  ", bdata(h->value.string));
    }
    printf("\n");

    int cols = 0;
    int rows = DB_counts(table, &cols);
    check(rows != -1, "Invalid query results, probably not in table format.");

    for(row_i = 0; row_i < rows; row_i++) {
        for(col_i = 0; col_i < cols; col_i++) {
            tns_value_t *col = DB_get(table, row_i, col_i);
            print_datum(col);
            printf("  ");
        }
        printf("\n");
    }
    printf("\n");

error: // fallthrough
    return;
}

void display_response(const char *msg, size_t len)
{
    tns_value_t *resp = tns_parse(msg, len, NULL);
    hnode_t *node = NULL;

    check(tns_get_type(resp) == tns_tag_dict, "Server returned an invalid response, must be a dict.");

    node = hash_lookup(resp->value.dict, &ERROR_KEY);

    if(node) {
        tns_value_t *val = hnode_get(node);
        printf("ERROR: %s\n", bdata(val->value.string));
    } else {
        node = hash_lookup(resp->value.dict, &HEADERS_KEY);
        check(node != NULL, "Server returned an invalid response, need a 'headers'.");
        tns_value_t *headers = hnode_get(node);
        check(tns_get_type(headers) == tns_tag_list, "Headers must be a list, server is screwed up.");

        node = hash_lookup(resp->value.dict, &ROWS_KEY);
        check(node != NULL, "Server returned an invalid response, need a 'rows'.");
        tns_value_t *rows = hnode_get(node);
        check(tns_get_type(rows) == tns_tag_list, "Rows must be a list, server is screwed up.");

        if(darray_end(rows->value.list) == 1) {
            display_map_style(headers, rows);
        } else {
            display_table_style(headers, rows);
        }
    }

error: // fallthrough
    tns_value_destroy(resp);
    return;
}

int send_recv_control(bstring args, void *socket)
{
    int rc = 0;
    zmq_msg_t *outmsg = NULL;
    zmq_msg_t *inmsg = NULL;

    outmsg = calloc(sizeof(zmq_msg_t), 1);
    check_mem(outmsg);
    inmsg = calloc(sizeof(zmq_msg_t), 1);
    check_mem(inmsg);
    
    rc = zmq_msg_init(outmsg);
    check(rc == 0, "Failed to initialize outgoing message.");
    rc = zmq_msg_init(inmsg);
    check(rc == 0, "Failed to initialize incoming message.");

    bstring request = parse_input(args);
    check(request != NULL, "Invalid command, try again.");

    // send the message
    rc = zmq_msg_init_data(outmsg, bdata(request), blength(request)+1, bstring_free, request);
    check(rc == 0, "Failed to init outgoing message.");

    rc = mqsend(socket, outmsg, 0);
    check(rc == 0, "Failed to send message to control port.");
    free(outmsg);

    // recv the response
    rc = mqrecv(socket, inmsg, 0);
    check(rc == 0, "Failed to receive message from control port.");

    display_response((const char *)zmq_msg_data(inmsg), zmq_msg_size(inmsg));

    fflush(stdout);
    free(inmsg);

    return 0;

error:
    if(outmsg) free(outmsg);
    if(inmsg) free(inmsg);
    return -1;
}

int control_server(struct ServerRun *r, tns_value_t *res)
{
    int rc = 0;
    void *socket = NULL;
    bstring prompt = NULL;
    bstring control = NULL;
    r->ran = 1;
    int cols = 0;
    int rows = DB_counts(res, &cols);

    check(rows != -1, "Invalid data given to internal routine control_server.");
    check(rows == 1, "Ambiguous server select, expected 1 but got %d.", rows);

    bstring server_name = DB_get_as(res, 0, 0, string);
    bstring chroot = DB_get_as(res, 0, 1, string);

    check(server_name != NULL && chroot != NULL, 
            "Somehow didn't get a good server_name and chroot.");

    prompt = bformat("m2 [%s]> ", bdata(server_name));
    control = bformat("ipc://%s/run/control", bdata(chroot));
    log_info("Connecting to control port %s", bdata(control));

    mqinit(1);
    Register_init();

    socket = mqsocket(ZMQ_REQ);
    check(socket != NULL, "Failed to create REQ socket.");

    rc = zmq_connect(socket, bdata(control));
    check(rc == 0, "Failed to connect to control port.");

    rc = linenoise_runner(bdata(prompt), send_recv_control, socket);

    bdestroy(prompt);
    bdestroy(control);
    zmq_close(socket);
    zmq_term(ZMQ_CTX);
    return rc;

error:
    if(prompt) bdestroy(prompt);
    if(control) bdestroy(control);
    if(socket) zmq_close(socket);
    return -1;
}

int Command_control(Command *cmd)
{
    return exec_server_operations(cmd, control_server, "name, chroot");
}

static int run_command(bstring line, void *ignored)
{
    bstring args = bformat("m2sh %s", bdata(line));
    int rc = Command_run(args);

    bdestroy(args);
    return rc;
}

int Command_shell(Command *cmd)
{
    return linenoise_runner("mongrel2> ", run_command, NULL);
}

