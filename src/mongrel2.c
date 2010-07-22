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

#include <server.h>
#include <dbg.h>
#include <task/task.h>
#include <string.h>
#include <config/config.h>
#include <adt/list.h>
#include <config/db.h>
#include <unixy.h>
#include <time.h>
#include <dir.h>
#include <signal.h>
#include <mime.h>


FILE *LOG_FILE = NULL;

extern int RUNNING;
int RELOAD;

struct tagbstring PRIV_DIR = bsStatic("/");

Server *SERVER = NULL;

void terminate(int s)
{
    RUNNING=0;
    RELOAD = s == SIGHUP;
    if(!RELOAD) {
        debug("SHUTDOWN REQUESTED.");
        fdclose(SERVER->listen_fd);
    } else {
        debug("RELOAD RECEIVED, I'll do it on the next request.");
    }
}

void start_terminator()
{
    struct sigaction sa, osa;
    memset(&sa, 0, sizeof sa);
    sa.sa_handler = terminate;
    sa.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa, &osa);
    sigaction(SIGTERM, &sa, &osa);
    sigaction(SIGHUP, &sa, &osa);
}


Server *load_server(const char *db_file, const char *server_name)
{
    int rc = 0;
    list_t *servers = Config_load_servers(db_file, server_name);

    check(servers, "Failed to load server config from %s for host %s", db_file, server_name);
    check(list_count(servers) == 1, "Currently only support running one server.");

    Server *srv = lnode_get(list_first(servers));

    rc = Config_load_mimetypes();
    check(rc == 0, "Failed to load mime types.");

    list_destroy_nodes(servers);
    list_destroy(servers);

    DB_close();

    return srv;
error:

    return NULL;
}


int clear_pid_file(Server *srv)
{
    bstring pid_file = bformat("%s%s", bdata(srv->chroot), bdata(srv->pid_file));

    int rc = Unixy_remove_dead_pidfile(pid_file);
    check(rc == 0, "Failed to remove the dead PID file: %s", bdata(pid_file));
    bdestroy(pid_file);
    
    return 0;
error:
    return -1;
}


int attempt_chroot_drop(Server *srv)
{
    int rc = 0;

    if(Unixy_chroot(srv->chroot) == 0) {
        log_info("All loaded up, time to turn into a server.");

        check(access("/logs", F_OK) == 0, "logs directory doesn't exist in %s or isn't owned right.", bdata(srv->chroot));
        check(access("/run", F_OK) == 0, "run directory doesn't exist in %s or isn't owned right.", bdata(srv->chroot));

        rc = Unixy_daemonize();
        check(rc == 0, "Failed to daemonize, looks like you're hosed.");

        FILE *log = fopen(bdata(srv->error_log), "a+");
        check(log, "Couldn't open %s log file.", bdata(srv->error_log));
        setbuf(log, NULL);

        LOG_FILE = log;

        rc = Unixy_pid_file(srv->pid_file);
        check(rc == 0, "Failed to make the PID file %s", bdata(srv->pid_file));

        rc = Unixy_drop_priv(&PRIV_DIR);
        check(rc == 0, "Failed to drop priv to the owner of %s", bdata(&PRIV_DIR));

    } else {
        log_err("Couldn't chroot too %s, assuming running in test mode.", bdata(srv->chroot));
    }
    return 0;

error:
    return -1;
}

void final_setup()
{
    start_terminator();
    Server_init();
}


void shutdown_handlers(void *value, void *data)
{
    Route *route = (Route *)value;
    Backend *bk = (Backend *)route->data;

    if(bk->type == BACKEND_HANDLER) {
        Handler *handler = bk->target.handler;
        handler->running = 0;

        if(tasknuke(taskgetid(handler->task)) == 0) {
            taskready(handler->task);
        }
    }
}

void close_handlers(void *value, void *data)
{
    Route *route = (Route *)value;
    Backend *bk = (Backend *)route->data;

    if(bk->type == BACKEND_HANDLER) {
        Handler *handler = bk->target.handler;
        zmq_close(handler->send_socket); handler->send_socket = NULL;
        zmq_close(handler->recv_socket); handler->recv_socket = NULL;
    }
}


void stop_handlers(Server *srv)
{
    tst_traverse(srv->default_host->routes->routes, shutdown_handlers, NULL);
    taskyield();
    taskdelay(100);
    tst_traverse(srv->default_host->routes->routes, close_handlers, NULL);
}


Server *reload_server(Server *old_srv, const char *db_file, const char *server_name)
{
    RUNNING = 1;

    MIME_destroy();

    stop_handlers(old_srv);

    Server *srv = load_server(db_file, server_name);
    check(srv, "Failed to load new server config.");

    srv->listen_fd = dup(old_srv->listen_fd);

    fdclose(SERVER->listen_fd);

    return srv;

error:
    return NULL;
}


void complete_shutdown(Server *srv)
{
    fdclose(srv->listen_fd);
    stop_handlers(srv);

    int left = taskwaiting();

    debug("Waiting for connections to die: %d", left);
    while((left = taskwaiting()) > 0) {
        // TODO: after a certain time close all of the connection sockets forcefully
        taskdelay(1000);
        debug("Waiting for connections to die: %d", left);
    }

    MIME_destroy();
    Server_destroy(srv);
    zmq_term(ZMQ_CTX);
    taskexitall(0);
}



void taskmain(int argc, char **argv)
{
    LOG_FILE = stderr;
    int rc = 0;

    check(argc == 3, "usage: server config.sqlite default_host");

    SERVER = load_server(argv[1], argv[2]);
    check(SERVER, "Aborting since can't load server.");

    rc = clear_pid_file(SERVER);
    check(rc == 0, "PID file failure, aborting rather than trying to start.");

    rc = attempt_chroot_drop(SERVER);
    check(rc == 0, "Major failure in chroot/droppriv, aborting."); 

    final_setup();

    while(1) {
        Server_start(SERVER);

        if(RELOAD) {
            log_info("Reload requested, will load %s from %s", argv[1], argv[2]);
            Server *new_srv = reload_server(SERVER, argv[1], argv[2]);
            check(new_srv, "Failed to load the new configuration, will keep the old one.");

            // for this to work handlers need to die more gracefully
            
            SERVER = new_srv;
        } else {
            log_info("Shutdown requested, goodbye.");
            break;
        }
    }

    complete_shutdown(SERVER);
    return;

error:
    log_err("Exiting due to error.");
    taskexitall(1);
}

