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

#include "config/config.h"

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <sqlite3.h>

#include "adt/tst.h"
#include "config/db.h"
#include "dir.h"
#include "dbg.h"
#include "mime.h"
#include "proxy.h"
#include "server.h"
#include "setting.h"

#define arity(N) check(cols == (N), "Wrong number of cols: expected %d got %d", cols, (N))
#define SQL(Q, ...) sqlite3_mprintf((Q), ##__VA_ARGS__)
#define SQL_FREE(Q) if(Q) sqlite3_free(Q)

static tst_t *LOADED_HANDLERS = NULL;
static tst_t *LOADED_PROXIES = NULL;

static int Config_load_handler_cb(void *param, int cols, char **data, char **names)
{
    arity(5);

    Handler *handler = Handler_create(data[1], data[2], data[3], data[4]);
    check(handler != NULL, "Loaded handler %s with send_spec=%s send_ident=%s recv_spec=%s recv_ident=%s", data[0], data[1], data[2], data[3], data[4]);

    log_info("Loaded handler %s with send_spec=%s send_ident=%s recv_spec=%s recv_ident=%s", data[0], data[1], data[2], data[3], data[4]);

    LOADED_HANDLERS = tst_insert(LOADED_HANDLERS, data[0], strlen(data[0]), handler);

    return 0;

error:
    return -1;
}

static int Config_load_handlers()
{
    const char *HANDLER_QUERY = "SELECT id, send_spec, send_ident, recv_spec, recv_ident FROM handler";

    int rc = DB_exec(HANDLER_QUERY, Config_load_handler_cb, NULL);
    check(rc == 0, "Failed to load handlers");

    return 0;

error:
    return -1;
}

static int Config_load_proxy_cb(void *param, int cols, char **data, char **names)
{
    arity(3);

    Proxy *proxy = Proxy_create(bfromcstr(data[1]), atoi(data[2]));
    check(proxy != NULL, "Failed to create proxy %s with address=%s port=%s", data[0], data[1], data[2]);

    log_info("Loaded proxy %s with address=%s port=%s", data[0], data[1], data[2]);

    LOADED_PROXIES = tst_insert(LOADED_PROXIES, data[0], strlen(data[0]), proxy);

    return 0;

error:
    return -1;
}

static int Config_load_proxies()
{
    const char *PROXY_QUERY = "SELECT id, addr, port FROM proxy";

    int rc = DB_exec(PROXY_QUERY, Config_load_proxy_cb, NULL);
    check(rc == 0, "Failed to load proxies");

    return 0;

error:
    return -1;
}

static int Config_load_dir_cb(void *param, int cols, char **data, char **names)
{
    arity(5);

    void **target = (void **)param;
    *target = Dir_create(data[1], data[2], data[3], data[4]);
    check(*target, "Failed to create dir %s with base=%s prefix=%s index=%s def_ctype=%s", data[0], data[1], data[2], data[3], data[4]);

    log_info("Created dir %s with base=%s prefix=%s index=%s def_ctype=%s", data[0], data[1], data[2], data[3], data[4]);

    return 0;

error:
    return -1;
}

static int Config_load_route_cb(void *param, int cols, char **data, char **names)
{
    arity(4);

    Host *host = (Host*)param;
    check(host, "Expected host as param");

    void *target = NULL;
    BackendType type = 0;

    const char *DIR_QUERY = "SELECT directory.id as id, base, route.path as prefix, index_file, default_ctype "
        "FROM route, directory "
        "WHERE directory.id = target_id AND target_type='dir' AND target_id=%s AND route.id=%s";

    if(strcmp("handler", data[3]) == 0)
    {
        Handler *handler = tst_search(LOADED_HANDLERS, data[2], strlen(data[2]));
        check(handler, "Failed to find handler %s for route %s:%s", data[2], data[0], data[1]);

        log_info("Created handler route %s:%s -> %s:%s", data[0], data[1], bdata(handler->send_ident), bdata(handler->send_spec));
        target = handler;

        type = BACKEND_HANDLER;
    }
    else if(strcmp("proxy", data[3]) == 0)
    {
        Proxy *proxy = tst_search(LOADED_PROXIES, data[2], strlen(data[2]));
        check(proxy, "Failed to find proxy %s for route %s:%s", data[2], data[0], data[1]);

        log_info("Created proxy route %s:%s -> %s:%d", data[0], data[1], bdata(proxy->server), proxy->port);
        target = proxy;

        type = BACKEND_PROXY;
    }
    else if(strcmp("dir", data[3]) == 0)
    {
        char *query = SQL(DIR_QUERY, data[2], data[0]);
        int rc = DB_exec(query, Config_load_dir_cb, &target);
        SQL_FREE(query);

        check(rc == 0, "Failed to find dir for route %s:%s", data[0], data[1]);

        type = BACKEND_DIR;
    }
    else
    {
        sentinel("Invalid handler type %s for host %s", data[3], bdata(host->name));
    }

    Host_add_backend(host, data[1], strlen(data[1]), type, target);

    return 0;

error:
    return -1;
}

static int Config_load_host_cb(void *param, int cols, char **data, char **names)
{
    arity(4);

    char *query = NULL;
    Server *server = (Server*)param;
    check(server, "Expected server as param");

    Host *host = Host_create(data[1]);
    check(host != NULL, "Failed to create host %s with %s", data[0], data[1]);

    const char *ROUTE_QUERY = "SELECT route.id, route.path, route.target_id, route.target_type "
        "FROM route, host WHERE host_id=%s AND "
        "host.server_id=%s AND host.id = route.host_id";
    query = SQL(ROUTE_QUERY, data[0], data[3]);

    int rc = DB_exec(query, Config_load_route_cb, host);
    check(rc == 0, "Failed to load routes for host %s:%s", data[0], data[1]);

    log_info("Adding host %s:%s to server at pattern %s", data[0], data[1], data[2]);

    Server_add_host(server, bfromcstr(data[2]), host);

    if(biseq(host->name, server->default_hostname)) {
        debug("Setting default host to host %s:%s", data[0], data[1]);
        server->default_host = host;
    }

    SQL_FREE(query);
    return 0;

error:
    SQL_FREE(query);
    return -1;
}

static int Config_load_server_cb(void* param, int cols, char **data, char **names)
{
    arity(8);

    Server **server = (Server **)param;
    if(*server != NULL)
    {
        log_info("More than one server object matches given uuid, using last found");
        Server_destroy(*server);
    }

    *server = Server_create(data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
    check(*server, "Failed to create server %s:%s on port %s", data[0], data[2], data[3]);


    const char *HOST_QUERY = "SELECT id, name, matching, server_id FROM host WHERE server_id = %s";
    char *query = SQL(HOST_QUERY, data[0]);
    check(query, "Failed to craft query string");

    int rc = DB_exec(query, Config_load_host_cb, *server);
    check(rc == 0, "Failed to find hosts for server %s:%s on port %s", data[0], data[1], data[2]);

    log_info("Loaded server %s:%s on port %s with default host %s", data[0], data[1], data[3], data[2]); 

    SQL_FREE(query);

    return 0;

error:
    Server_destroy(*server);
    SQL_FREE(query);
    return -1;

}

Server *Config_load_server(const char *uuid)
{
    Config_load_handlers();
    Config_load_proxies();

    const char *SERVER_QUERY = "SELECT id, uuid, default_host, port, chroot, access_log, error_log, pid_file FROM server WHERE uuid=%Q";
    char *query = SQL(SERVER_QUERY, uuid);

    Server *server = NULL;
    int rc = DB_exec(query, Config_load_server_cb, &server);
    check(rc == 0, "Failed to select server with uuid %s", uuid);

    SQL_FREE(query);

    return server;

error:
    SQL_FREE(query);
    return NULL;
}

static int Config_load_mimetypes_cb(void *param, int cols, char **data, char **names)
{
    arity(3);

    int rc = MIME_add_type(data[1], data[2]);
    check(rc == 0, "Failed to create mimetype %s:%s from id %s", data[1], data[2], data[0]);

    return 0;

error:
    return -1;
}

int Config_load_mimetypes()
{
    const char *MIME_QUERY = "SELECT id, extension, mimetype FROM mimetype";

    int rc = DB_exec(MIME_QUERY, Config_load_mimetypes_cb, NULL);
    check(rc == 0, "Failed to load mimetypes");

    return 0;

error:
    return -1;
}

static int Config_load_settings_cb(void *param, int cols, char **data, char **names)
{
    arity(3);

    int rc = Setting_add(data[1], data[2]);
    check(rc == 0, "Failed to create setting %s:%s from id %s", data[1], data[2], data[0]);

    return 0;

error:
    return -1;
}

int Config_load_settings()
{
    const char *SETTINGS_QUERY = "SELECT id, key, value FROM setting";

    int rc = DB_exec(SETTINGS_QUERY, Config_load_settings_cb, NULL);
    check(rc == 0, "Failed to load settings");

    return 0;

error:
    return -1;
}

int Config_init_db(const char *path)
{
    return DB_init(path);
}

void Config_close_db()
{
    DB_close();
}



static void shutdown_handlers(void *value, void *data)
{
    Handler *handler = (Handler *)value;
    assert(handler && "Why? Handler is NULL.");

    if(handler->running) {
        handler->running = 0;

        if(tasknuke(taskgetid(handler->task)) == 0) {
            taskready(handler->task);
        }
    }
}

static void close_handlers(void *value, void *data)
{
    Handler *handler = (Handler *)value;
    assert(handler && "Why? Handler is NULL.");


    if(handler->send_socket) {
        zmq_close(handler->send_socket);
        handler->send_socket = NULL;
    }

    if(handler->recv_socket) {
        zmq_close(handler->recv_socket);
        handler->recv_socket = NULL;
    }

    Handler_destroy(handler);
}

void Config_stop_handlers()
{
    tst_traverse(LOADED_HANDLERS, shutdown_handlers, NULL);
    taskyield();
    taskdelay(100);
    tst_traverse(LOADED_HANDLERS, close_handlers, NULL);

    tst_destroy(LOADED_HANDLERS);

    LOADED_HANDLERS = NULL;
}

static void stop_proxy(void *value, void *data)
{
    Proxy_destroy((Proxy *)value);
}

void Config_stop_proxies()
{
    tst_traverse(LOADED_PROXIES, stop_proxy, NULL);

    tst_destroy(LOADED_PROXIES);
    LOADED_PROXIES = NULL;
}

