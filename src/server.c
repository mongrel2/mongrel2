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

#include <dbg.h>
#include <task/task.h>
#include <connection.h>
#include <register.h>
#include <server.h>
#include <host.h>
#include <assert.h>
#include <string.h>
#include <mem/halloc.h>
#include <routing.h>
#include <setting.h>

int RUNNING=1;

void host_destroy_cb(Route *r, RouteMap *map)
{
    if(r->data) {
        Host_destroy((Host *)r->data);
        r->data = NULL;
    }
}

Server *Server_create(const char *uuid, const char *port,
        const char *chroot, const char *access_log,
        const char *error_log, const char *pid_file)
{
    Server *srv = h_calloc(sizeof(Server), 1);
    check_mem(srv);

    srv->hosts = RouteMap_create(host_destroy_cb);
    check(srv->hosts, "Failed to create host RouteMap.");

    srv->port = atoi(port);
    check(port > 0, "Can't bind to the given port: %s", port);

    srv->listen_fd = 0;

    srv->uuid = bfromcstr(uuid); check_mem(srv->uuid);
    srv->chroot = bfromcstr(chroot); check_mem(srv->chroot);
    srv->access_log = bfromcstr(access_log); check_mem(srv->access_log);
    srv->error_log = bfromcstr(error_log); check_mem(srv->error_log);
    srv->pid_file = bfromcstr(pid_file); check_mem(srv->pid_file);

    return srv;

error:
    Server_destroy(srv);
    return NULL;
}



void Server_destroy(Server *srv)
{
    if(srv) {
        RouteMap_destroy(srv->hosts);
        bdestroy(srv->uuid);
        bdestroy(srv->chroot);
        bdestroy(srv->access_log);
        bdestroy(srv->error_log);
        bdestroy(srv->pid_file);

        fdclose(srv->listen_fd);
        h_free(srv);
    }
}


void Server_init()
{
    int mq_threads = Setting_get_int("zeromq.threads", 1);

    if(mq_threads > 1) {
        log_info("WARNING: Setting zeromq.threads greater than 1 can cause lockups in your handlers.");
    }

    log_info("Starting 0MQ with %d threads.", mq_threads);
    mqinit(mq_threads);
    Register_init();
    Request_init();
    Connection_init();
}


void handlers_receive_start(void *value, void *data)
{
    Route *route = (Route *)value;
    if(route) { 
        Backend *found = (Backend *)route->data;

        // TODO: make whether the handler is loaded immediately optional
        if(found->type == BACKEND_HANDLER) {
            debug("LOADING BACKEND %s", bdata(route->pattern));
            taskcreate(Handler_task, found->target.handler, HANDLER_STACK);
        }
    }
}

void Server_start(Server *srv)
{
    int cfd;
    int rport;
    char remote[IPADDR_SIZE];

    check(srv->default_host, "No default_host set.");

    taskname("SERVER");

    log_info("Starting server on port %d", srv->port);

    tst_traverse(srv->default_host->routes->routes, handlers_receive_start, srv);


    while(RUNNING && (cfd = netaccept(srv->listen_fd, remote, &rport)) >= 0) {
        debug("Connection from %s:%d to %s:%d", remote, rport, 
                bdata(srv->default_host->name), srv->port);

        Connection *conn = Connection_create(srv, cfd, rport, remote);
        Connection_accept(conn);
    }

    debug("SERVER EXITED with error: %s and return value: %d", strerror(errno), cfd);

    return;

error:
    log_err("Server startup failed, aborting.");
    taskexitall(1);
}



int Server_add_host(Server *srv, bstring pattern, Host *host)
{
    return RouteMap_insert_reversed(srv->hosts, pattern, host);
}


void Server_set_default_host(Server *srv, Host *host)
{
    srv->default_host = host;
}



Host *Server_match_backend(Server *srv, bstring target)
{
    // TODO: figure out the best matching policy, longest? first? all?
    Route *found = NULL;

    debug("Looking for target host: %s", bdata(target));

    list_t *results = RouteMap_match_suffix(srv->hosts, target);
    lnode_t *n = list_first(results);

    if(n) {
        found = lnode_get(n);
        assert(found && "RouteMap returned a list node with NULL.");
        debug("Found host at %s", bdata(found->pattern));
    }

    list_destroy_nodes(results);
    list_destroy(results);

    if(found) {
        assert(found->data && "Invalid value for stored route.");
        return found->data;
    } else {
        return NULL;
    }
}
