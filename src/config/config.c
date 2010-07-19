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

#include <config/config.h>
#include <config/db.h>
#include <stdlib.h>
#include <dbg.h>
#include <adt/list.h>
#include <server.h>
#include <string.h>
#include <stdarg.h>
#include <sqlite3.h>
#include <assert.h>
#include <mime.h>
#include <dir.h>


#define arity(N) check(cols == (N), "Wrong number of cols: %d but expect %d", cols, (N))
#define SQL(Q, ...) sqlite3_mprintf((Q), ##__VA_ARGS__)
#define SQL_FREE(Q) if(Q) sqlite3_free(Q)

int Config_dir_load_cb(void *param, int cols, char **data, char **names)
{
    arity(5);
    Dir **target = (Dir **)param;

    log_info("Loaded dir config for %s:%s", data[0], data[1]);

    *target = Dir_create(data[1], data[2], data[3], data[4]);

    check(*target, "Failed to create dir for %s:%s", data[0], data[1]);

    return 0;
error:
    return -1;
}

int Config_handler_load_cb(void *param, int cols, char **data, char **names)
{
    arity(5);
    Handler **target = (Handler **)param;

    log_info("Loaded handler config for %s:%s:%s:%s:%s", data[0], data[1], data[2],
            data[3], data[4]);

    *target = Handler_create(data[1], data[2], data[3], data[4]);

    check(*target, "Failed to create handler for %s:%s:%s:%s:%s", 
            data[0], data[1], data[2], data[3], data[4]);

    return 0;
error:
    return -1;
}

int Config_proxy_load_cb(void *param, int cols, char **data, char **names)
{
    arity(3);
    Proxy **target = (Proxy **)param;

    log_info("Loaded proxy config for %s:%s:%s", data[0], data[1], data[2]);

    *target = Proxy_create(bfromcstr(data[1]), atoi(data[2]));
    check(*target, "Failed to create proxy for %s:%s:%s", data[0], data[1], data[2]);

    return 0;
error:
    return -1;
}

int Config_route_load_cb(void *param, int cols, char **data, char **names)
{
    arity(4);
    
    Host *host = (Host *)param;
    check(host, "Should get a host on callback for routes.");

    char *query = NULL;
    int rc = 0;
    void *target = NULL;
    BackendType type = 0;
    const char *HANDLER_QUERY = "SELECT id, send_spec, send_ident, recv_spec, recv_ident FROM handler WHERE id=%s";
    const char *PROXY_QUERY = "SELECT id, addr, port FROM proxy WHERE id=%s";
    const char *DIR_QUERY = "SELECT directory.id as id, base, route.path as "
        "prefix, index_file, default_ctype FROM route, directory where "
        "directory.id=target_id and target_type='dir' and target_id=%s "
        "and route.id=%s";

    if(strcmp("handler", data[3]) == 0) {
        query = SQL(HANDLER_QUERY, data[2]);

        rc = DB_exec(query, Config_handler_load_cb, &target);
        check(rc == 0, "Failed to find handler for route: %s (id: %s)",
                data[1], data[0]);

        log_info("ROUTE(%s) %s -> %s:%s (handler)", data[0], data[1],
                bdata(((Handler *)target)->send_ident),
                bdata(((Handler*)target)->send_spec));

        type = BACKEND_HANDLER;

    } else if(strcmp("proxy", data[3]) == 0) {
        query = SQL(PROXY_QUERY, data[2]);
        rc = DB_exec(query, Config_proxy_load_cb, &target);
        check(rc == 0, "Failed to find proxy for route: %s (id: %s)",
                data[1], data[0]);

        log_info("ROUTE(%s) %s -> %s:%d (proxy)", data[0], data[1],
                bdata(((Proxy*)target)->server),
                ((Proxy*)target)->port);

        type = BACKEND_PROXY;

    } else if(strcmp("dir", data[3]) == 0) {
        query = SQL(DIR_QUERY, data[2], data[0]);
        rc = DB_exec(query, Config_dir_load_cb, &target);
        check(rc == 0, "Failed to find dir for route: %s (id: %s)",
                data[1], data[0]);
        type = BACKEND_DIR;
        
    } else {
        sentinel("Invalid handler type: %s for host: %s",
                data[3], bdata(host->name));
    }

    Host_add_backend(host, data[1], strlen(data[1]), type, target);

    SQL_FREE(query);

    return 0;
error:

    SQL_FREE(query);
    return -1;
}

int Config_host_load_cb(void *param, int cols, char **data, char **names)
{
    arity(3);

    Server *srv = (Server *)param;
    char *query = NULL;

    const char *ROUTE_QUERY = "SELECT id, path, target_id, target_type FROM route WHERE host_id=%s";
    check(srv, "Should be given the server to add hosts to.");

    Host *host = Host_create(data[1]);
    check(host, "Failed to create host: %s (id: %s)", data[1], data[0]);

    query = SQL(ROUTE_QUERY, data[0]);

    DB_exec(query, Config_route_load_cb, host);

    log_info("Adding host %s (id: %s) to server at pattern %s", data[1], data[0], data[2]);


    Server_add_host(srv, bfromcstr(data[2]), host);

    if(srv->default_host == NULL) Server_set_default_host(srv, host);

    SQL_FREE(query);
    return 0;
error:

    SQL_FREE(query);
    return -1;
}

int Config_mimetypes_load_cb(void *param, int cols, char **data, char **names)
{
    arity(3);

    int rc = MIME_add_type(data[1], data[2]);
    check(rc == 0, "Failed to add mimetype %s:%s:%s", data[0], data[1], data[2]);

    return 0;
error:
    return -1;
}

int Config_server_load_cb(void *param, int cols, char **data, char **names)
{
    arity(8);

    list_t *servers = (list_t *)param;
    Server *srv = NULL;
    const char *HOST_QUERY = "SELECT id, name, matching FROM host where server_id = %s";
    char *query = NULL;
 
    log_info("Configuring server ID: %s, default host: %s, port: %s", 
            data[1], data[2], data[3]);

    srv = Server_create(data[1], data[3], data[4], data[5], data[6], data[7]);
    check(srv, "Failed to create server %s on port %s", data[2], data[3]);

    query = SQL(HOST_QUERY, data[0]);
    check(query, "Failed to craft query.");

    int rc = DB_exec(query, Config_host_load_cb, srv);
    check(rc == 0, "Failed to find hosts for server %s on port %s (id: %s)",
            data[2], data[3], data[0]);

    list_append(servers, lnode_create(srv));

    SQL_FREE(query);
    return 0;

error:
    SQL_FREE(query);
    return -1;
}


list_t *Config_load_servers(const char *path, const char *name)
{
    list_t *servers = list_create(LISTCOUNT_T_MAX);
    const char *SERVER_QUERY = "SELECT id, uuid, default_host, port, chroot, access_log, error_log, pid_file FROM server WHERE default_host=%Q";
    char *query = NULL;

    int rc = DB_init(path);
    check(rc == 0, "Failed to load config database: %s", path);

    query = SQL(SERVER_QUERY, name);

    rc = DB_exec(query, Config_server_load_cb, servers);
    check(rc == 0, "Failed to select servers from: %s", path);

    SQL_FREE(query);

    return servers;
error:

    SQL_FREE(query);
    return NULL;
}


int Config_load_mimetypes()
{
    const char *MIME_QUERY = "SELECT id, extension, mimetype FROM mimetype";

    int rc = DB_exec(MIME_QUERY, Config_mimetypes_load_cb, NULL);
    check(rc == 0, "Failed to load mimetypes.");

    return 0;
error:
    return -1;
}
