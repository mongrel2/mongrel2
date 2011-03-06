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

#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <assert.h>

#include "server.h"
#include "dbg.h"
#include "dir.h"
#include "task/task.h"
#include "config/config.h"
#include "config/db.h"
#include "adt/list.h"
#include "unixy.h"
#include "mime.h"
#include "superpoll.h"
#include "setting.h"
#include "version.h"
#include "control.h"
#include "log.h"

FILE *LOG_FILE = NULL;

extern int RUNNING;
extern uint32_t THE_CURRENT_TIME_IS;
int RELOAD;
int MURDER;

struct tagbstring PRIV_DIR = bsStatic("/");

Server *SERVER = NULL;


void terminate(int s)
{
    MURDER = s == SIGTERM;
    switch(s)
    {
        case SIGHUP:
            RELOAD = 1;
            RUNNING = 0;
            log_info("RELOAD REQUESTED, I'll do it on the next request.");
            break;
        default:
            if(!RUNNING) {
                log_info("SIGINT CAUGHT AGAIN, ASSUMING MURDER.");
                MURDER = 1;
            } else {
                RUNNING = 0;
                log_info("SHUTDOWN REQUESTED: %s", MURDER ? "MURDER" : "GRACEFUL (SIGINT again to EXIT NOW)");
                fdclose(SERVER->listen_fd);
            }
            break;
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

    // try blocking SIGPIPE with this
    sigset_t x;
    sigemptyset(&x);
    sigaddset(&x, SIGPIPE);
    sigprocmask(SIG_BLOCK, &x, NULL);

    sigemptyset(&x);
    sigaddset(&x, SIGPIPE);
    pthread_sigmask(SIG_BLOCK, &x, NULL);

    sa.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &sa, &osa);
}


Server *load_server(const char *db_file, const char *server_uuid, int reuse_fd)
{
    int rc = 0;
    Server *srv =  NULL;

    rc = Config_init_db(db_file);
    check(rc == 0, "Failed to load config database at %s", db_file);
    
    rc = Config_load_settings();
    check(rc == 0, "Failed to load global settings.");

    srv = Config_load_server(server_uuid);
    check(srv, "Failed to load server %s from %s", server_uuid, db_file);

    rc = Config_load_mimetypes();
    check(rc == 0, "Failed to load mime types.");

    if(reuse_fd == -1) {
        srv->listen_fd = netannounce(TCP, bdata(srv->bind_addr), srv->port);
        check(srv->listen_fd >= 0, "Can't announce on TCP port %d", srv->port);
        check(fdnoblock(srv->listen_fd) == 0, "Failed to set listening port %d nonblocking.", srv->port);
    } else {
        srv->listen_fd = dup(reuse_fd);
        check(srv->listen_fd != -1, "Failed to dup the socket from the running server.");
        fdclose(reuse_fd);
    }

    Config_close_db();
    return srv;
error:
    Server_destroy(srv);
    Config_close_db();
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

void tickertask(void *v)
{
    // this will be used later for timeouts
    while(1) {
        THE_CURRENT_TIME_IS = time(NULL);
        taskdelay(5000);
    }
}

int attempt_chroot_drop(Server *srv)
{
    int rc = 0;

    if(Unixy_chroot(srv->chroot) == 0) {
        log_info("All loaded up, time to turn into a server.");

        check(access("/run", F_OK) == 0, "/run directory doesn't exist in %s or isn't owned right.", bdata(srv->chroot));
        check(access("/tmp", F_OK) == 0, "/tmp directory doesn't exist in %s or isn't owned right.", bdata(srv->chroot));

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

        // rewrite the access log to be in the right location
        bstring temp = bformat("%s%s", bdata(srv->chroot), bdata(srv->access_log));
        bassign(srv->access_log, temp);
        bdestroy(temp);

        temp = bformat(".%s", bdata(srv->pid_file));
        bassign(srv->pid_file, temp);
        bdestroy(temp);

        rc = Unixy_pid_file(srv->pid_file);
        check(rc == 0, "Failed to make the PID file %s", bdata(srv->pid_file));
    }

    return 0;

error:
    return -1;
}

void final_setup()
{
    start_terminator();
    Server_init();
    bstring end_point = bfromcstr("inproc://access_log");

    Log_init(bstrcpy(SERVER->access_log), end_point);
}



Server *reload_server(Server *old_srv, const char *db_file, const char *server_uuid)
{
    RUNNING = 1;

    MIME_destroy();

    Config_stop_handlers();
    Config_stop_proxies();
    Config_stop_dirs();
    Setting_destroy();

    Server *srv = load_server(db_file, server_uuid, old_srv->listen_fd);
    check(srv, "Failed to load new server config.");

    RELOAD = 0;
    return srv;

error:
    return NULL;
}


void complete_shutdown(Server *srv)
{
    fdclose(srv->listen_fd);
    Config_stop_handlers();
    Config_stop_proxies();
    Config_stop_dirs();

    int left = taskwaiting();

    // need -1 for the control port
    log_info("Waiting for connections to die: %d", left - 1);
    while((left = taskwaiting()) > 1 && !MURDER) {
        // TODO: after a certain time close all of the connection sockets forcefully
        taskdelay(3000);
        log_info("Waiting for connections to die: %d", left - 1);
    }

    MIME_destroy();

    log_info("Removing pid file %s", bdata(srv->pid_file));
    unlink((const char *)srv->pid_file->data);

    Server_destroy(srv);
    Control_port_stop();
    Log_term();

    zmq_term(ZMQ_CTX);

    Setting_destroy();

    taskexitall(0);
}



void taskmain(int argc, char **argv)
{
    LOG_FILE = stderr;
    int rc = 0;

    check(argc == 3, "usage: mongrel2 config.sqlite server_uuid");


    SERVER = load_server(argv[1], argv[2], -1);
    check(SERVER, "Aborting since can't load server.");

    SuperPoll_get_max_fd();

    rc = clear_pid_file(SERVER);
    check(rc == 0, "PID file failure, aborting rather than trying to start.");

    rc = attempt_chroot_drop(SERVER);
    check(rc == 0, "Major failure in chroot/droppriv, aborting."); 

    final_setup();

    Control_port_start();
    taskcreate(tickertask, NULL, 16 * 1024);

    while(1) {
        log_info("Starting " VERSION ". Copyright (C) Zed A. Shaw. Licensed BSD.");
        Server_start(SERVER);

        if(RELOAD) {
            log_info("Reload requested, will load %s from %s", argv[2], argv[1]);
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

