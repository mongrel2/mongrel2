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
#include <signal.h>

#include <sqlite3.h>

#include "adt/tst.h"
#include "dir.h"
#include "dbg.h"
#include "mime.h"
#include "proxy.h"
#include "server.h"
#include "setting.h"
#include "config/module.h"
#include "config/db.h"
#include "filter.h"
#include <dlfcn.h>

#include "xrequest.h"

Handler *Config_load_handler(int handler_id)
{
    tns_value_t *res = CONFIG_MODULE.load_handler(handler_id);

    DB_check(res, 0, 7,
            tns_tag_number, tns_tag_string, tns_tag_string, tns_tag_string,
            tns_tag_string, tns_tag_number, tns_tag_string);

    Handler *handler = Handler_create(
            DB_get_as(res, 0, 1, string), // send_spec
            DB_get_as(res, 0, 2, string), // send_ident
            DB_get_as(res, 0, 3, string), // recv_spec
            DB_get_as(res, 0, 4, string) // recv_ident
            );
    check(handler != NULL, "Failed to create handler id %d.", handler_id);

    if(DB_get_as(res, 0, 5, number) == 1) {
        handler->raw = 1;
    }

    if(biseqcstr(DB_get_as(res, 0, 6, string), "tnetstring")) {
        handler->protocol = HANDLER_PROTO_TNET;
    }

    log_info("Loaded handler %d:%s:%s:%s:%s",
            handler_id, bdata(handler->send_spec),
            bdata(handler->send_ident),
            bdata(handler->recv_spec),
            bdata(handler->recv_ident));

    tns_value_destroy(res);
    return handler;
error:
    if(res) tns_value_destroy(res);
    return NULL;
}


Proxy *Config_load_proxy(int proxy_id)
{
    tns_value_t *res = CONFIG_MODULE.load_proxy(proxy_id);

    DB_check(res, 0, 3,
            tns_tag_number, tns_tag_string, tns_tag_number);

    Proxy *proxy = Proxy_create(
            DB_get_as(res, 0, 1, string),
            DB_get_as(res, 0, 2, number)
            );
    check(proxy != NULL, "Failed to create proxy with id %d.", proxy_id);

    log_info("Loaded proxy %d:%s:%d",
            proxy_id, bdata(proxy->server), proxy->port);

    tns_value_destroy(res);
    return proxy;
error:
    if(res) tns_value_destroy(res);
    return NULL;
}


Dir *Config_load_dir(int dir_id)
{
    tns_value_t *res = CONFIG_MODULE.load_dir(dir_id);

    DB_check(res, 0, 5,
            tns_tag_number, tns_tag_string, tns_tag_string,
            tns_tag_string, tns_tag_number);

    Dir *dir = Dir_create(
            DB_get_as(res, 0, 1, string), // base
            DB_get_as(res, 0, 2, string), // index_file
            DB_get_as(res, 0, 3, string), // default_ctype
            DB_get_as(res, 0, 4, number) // cache_ttl
            );
    check(dir != NULL, "Failed to create directory id %d.", dir_id);

    log_info("Loaded directory %d:%s:%s",
            dir_id, bdata(dir->base), bdata(dir->index_file));

    tns_value_destroy(res);
    return dir;
error:
    if(res) tns_value_destroy(res);
    return NULL;
}

static inline Handler *Config_push_unique_handler(Server *srv, Handler *handler)
{
    int i = 0;

    // roll through the list of handlers and break if we find one
    // this is because, unlike other backends, handlers can't have
    // the same send/recv idents.
    for(i = 0; i < darray_end(srv->handlers); i++) {
        Handler *test = darray_get(srv->handlers, i);
        int same_send = biseq(test->send_spec, handler->send_spec);
        int same_recv = biseq(test->recv_spec, handler->recv_spec);

        if(same_send && same_recv) {
            Handler_destroy(handler);
            // WARNING: this breaks out and ends the loop
            return test;
        } else if(same_send) {
            log_warn("You have two handlers with the same send_spec: %s",
                    bdata(handler->send_spec));
        } else if(same_recv) {
            log_warn("You have two handlers with the same recv_spec: %s",
                    bdata(handler->recv_spec));
        }
    }

    // nothing was found since the loop completed, so add this one
    // and return it as the one the caller should use
    darray_push(srv->handlers, handler);
    return handler;
}

int Config_load_routes(Server *srv, Host *host, int host_id, int server_id)
{
    tns_value_t *res = CONFIG_MODULE.load_routes(host_id, server_id);
    int cols = 0;
    int rows = DB_counts(res, &cols);
    int row_i = 0;
    int rc = 0;

    if(rows == 0) {
        log_warn("Server has no routes, it won't do anything without at least one.");
        tns_value_destroy(res);
        return 0;
    }

    for(row_i = 0; row_i < rows; row_i++) {
        DB_check(res, row_i, 4, 
            tns_tag_number, tns_tag_string, tns_tag_number, tns_tag_string);
 
        int route_id = DB_get_as(res, row_i, 0, number);
        bstring path = DB_get_as(res, row_i, 1, string);
        int id = DB_get_as(res, row_i, 2, number);
        bstring type = DB_get_as(res, row_i, 3, string);
        void *target = NULL;
        BackendType backend_type = 0;

        if(biseqcstr(type, "dir")) {
            target = Config_load_dir(id);
            backend_type = BACKEND_DIR;
        } else if(biseqcstr(type, "proxy")) {
            target = Config_load_proxy(id);
            backend_type = BACKEND_PROXY;
        } else if(biseqcstr(type, "handler")) {
            Handler *temp = Config_load_handler(id);
            target = Config_push_unique_handler(srv, temp);
            check(target != NULL, "Failure pushing handler %d:%s.", id, bdata(path));
            backend_type = BACKEND_HANDLER;
        } else {
            sentinel("Invalid backend type: %s for route %d:%s.",
                    bdata(type), route_id, bdata(path));
        }

        check(target != NULL, "Failed to load backend type: %s for route %d:%s.",
                bdata(type), route_id, bdata(path));
        
        // add it to the route map for this host
        rc = Host_add_backend(host, path, backend_type, target);
        check(rc == 0, "Failed to add route %s to host %s.",
                bdata(path), bdata(host->name));

        log_info("Loaded route %d:%s:%s for host %d:%s",
                route_id, bdata(path), bdata(type),
                host_id, bdata(host->name));
    }

    tns_value_destroy(res);
    return row_i;

error:
    if(res) tns_value_destroy(res);
    return -1;
}


int Config_load_plugins(Server *srv, tns_value_t *res, int (*load_func)(Server *, bstring, tns_value_t *))
{
    int cols = 0;
    int rows = DB_counts(res, &cols);
    int row_i = 0;
    int rc = 0;

    for(row_i = 0; row_i < rows; row_i++) {
        DB_check(res, row_i, 3, 
            tns_tag_number, tns_tag_string, tns_tag_string);

        int id = DB_get_as(res, row_i, 0, number);
        bstring filter_name = DB_get_as(res, row_i, 1, string);
        bstring raw_settings = DB_get_as(res, row_i, 2, string);

        char *remain = NULL;
        tns_value_t *config = tns_parse(bdata(raw_settings), blength(raw_settings), &remain);

        check(config != NULL, "Failed to parse the settings for Filter '%s' id='%d'",
                bdata(filter_name), id);
        check(tns_get_type(config) == tns_tag_dict,
                "Settings for a filter must be a dict.");

        rc = load_func(srv, filter_name, config);
        check(rc == 0, "Failed to load plugin '%s' id='%d'", bdata(filter_name), id);
    }

    tns_value_destroy(res);
    return 0;

error:
    if(res) tns_value_destroy(res);
    return 1;
}

int Config_load_filters(Server *srv, int server_id)
{
    tns_value_t *res = CONFIG_MODULE.load_filters(server_id);
    return Config_load_plugins(srv,res,Filter_load);
}

int Config_load_xrequests(Server *srv, int server_id)
{
    tns_value_t *res = CONFIG_MODULE.load_xrequests(server_id);
    return Config_load_plugins(srv,res,Xrequest_load);
}

int Config_load_hosts(Server *srv, int server_id)
{
    tns_value_t *res = CONFIG_MODULE.load_hosts(server_id);
    int cols = 0;
    int rows = DB_counts(res, &cols);
    int row_i = 0; 
    int rc = 0;

    if(rows == 0) {
        log_warn("No hosts configured for server, I hope you know what you're doing.");
        tns_value_destroy(res);
        return 0;
    }

    for(row_i = 0; row_i < rows; row_i++) {
        DB_check(res, row_i, 4, 
            tns_tag_number, tns_tag_string, tns_tag_string, tns_tag_number);

        Host *host = Host_create(
                DB_get_as(res, row_i, 1, string), // name
                DB_get_as(res, row_i, 2, string) // matching
                );
        check_mem(host);

        int host_id = DB_get_as(res, row_i, 0, number);
        rc = Config_load_routes(srv, host, host_id, server_id);
        check(rc != -1, "Failed to load routes for host %s.", bdata(host->name));

        rc = Server_add_host(srv, host);
        check(rc == 0, "Failed to add host %s:%s to server.",
                bdata(host->name), bdata(host->matching));

        if(biseq(srv->default_hostname, host->name)) {
            srv->default_host = host;
        }
    }

    log_info("Loaded %d hosts for server %d:%s", row_i, server_id, bdata(srv->uuid));

    tns_value_destroy(res);
    return 0;

error:
    if(res) tns_value_destroy(res);
    return -1;

}

Server *Config_load_server(const char *uuid)
{
    tns_value_t *res = CONFIG_MODULE.load_server(uuid);
    int rc = 0;

    DB_check(res, 0, 10,
            tns_tag_number, tns_tag_string, tns_tag_string, tns_tag_string, tns_tag_number,
            tns_tag_string, tns_tag_string, tns_tag_string, tns_tag_string, tns_tag_number);

    int server_id = DB_get_as(res, 0, 0, number); // id

    Server *srv = Server_create(
            DB_get_as(res, 0, 1, string), // uuid
            DB_get_as(res, 0, 2, string), // default_host
            DB_get_as(res, 0, 3, string), // bind_addr
            DB_get_as(res, 0, 4, number), // port
            DB_get_as(res, 0, 5, string), // chroot
            DB_get_as(res, 0, 6, string), // access_log
            DB_get_as(res, 0, 7, string), // error_log
            DB_get_as(res, 0, 8, string), // pid_file
            DB_get_as(res, 0, 9, number) // use_ssl
            );
    check(srv != NULL, "Failed to create server %s", uuid);


    rc = Config_load_hosts(srv, server_id);
    check(rc == 0, "Failed to load the hosts for server: %s", bdata(srv->uuid));

    rc = Config_load_filters(srv, server_id);
    check(rc == 0, "Failed to load the filters for server: %s", bdata(srv->uuid));

    rc = Config_load_xrequests(srv, server_id);
    check(rc == 0, "Failed to load the filters for server: %s", bdata(srv->uuid));

    tns_value_destroy(res);
    return srv;

error:
    if(res) tns_value_destroy(res);
    return NULL;
}


static int simple_query_run(tns_value_t *res,
        int (*callback)(const char *key, const char *value))
{
    int row_i = 0;
    int cols = 0;
    int rows = DB_counts(res, &cols);
    check(rows != -1, "Results are not a table.");

    if(rows > 0) {
        for(row_i = 0; row_i < rows; row_i++) {
            DB_check(res, row_i, 3,
                    tns_tag_number, tns_tag_string, tns_tag_string);

            bstring key = DB_get_as(res, row_i, 1, string);
            bstring value = DB_get_as(res, row_i, 2, string);

            int rc = callback(bdata(key), bdata(value));
            check_debug(rc == 0, "Load callback failed.");
        }
    }

    return row_i;
error:
    return -1;
}

int Config_load_mimetypes()
{
    int rc = -1;
    tns_value_t *res = CONFIG_MODULE.load_mimetypes();
    check(tns_get_type(res) == tns_tag_list, "Wrong type, expected valid rows.");

    rc = simple_query_run(res, MIME_add_type);
    check(rc != -1, "Failed adding your settings, look at previous errors for clues.");

error: // fallthrough
    if(res) tns_value_destroy(res);
    return rc;
}


int Config_load_settings()
{
    tns_value_t *res = CONFIG_MODULE.load_settings();
    int rc = -1;
    check(tns_get_type(res) == tns_tag_list, "Wrong type, expected valid rows.");
    rc = simple_query_run(res, Setting_add);
    check(rc != -1, "Failed adding your settings, look at previous errors for clues.");

error: // falthrough
    if(res) tns_value_destroy(res);
    return rc;
}

int Config_init_db(const char *path)
{
    return CONFIG_MODULE.init(path);
}

void Config_close_db()
{
    CONFIG_MODULE.close();
}

#define SET_MODULE_FUNC(N)    CONFIG_MODULE.N = dlsym(lib, "config_" #N);\
    check(CONFIG_MODULE.N != NULL, "Config module %s doesn't have an " #N " function.", load_path);


int Config_module_load(const char *load_path)
{
    void *lib = dlopen(load_path, RTLD_LAZY | RTLD_LOCAL);
    check(lib != NULL, "Failed to load config module %s: %s.", load_path, dlerror());

    SET_MODULE_FUNC(init);
    SET_MODULE_FUNC(close);
    SET_MODULE_FUNC(load_handler);
    SET_MODULE_FUNC(load_proxy);
    SET_MODULE_FUNC(load_dir);
    SET_MODULE_FUNC(load_routes);
    SET_MODULE_FUNC(load_hosts);
    SET_MODULE_FUNC(load_server);
    SET_MODULE_FUNC(load_mimetypes);
    SET_MODULE_FUNC(load_settings);
    SET_MODULE_FUNC(load_filters);

    return 0;
error:
    return -1;
}

