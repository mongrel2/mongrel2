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
#include <sys/socket.h>

#include "server.h"
#include "dbg.h"
#include "dir.h"
#include "task/task.h"
#include "config/config.h"
#include "config/db.h"
#include "adt/darray.h"
#include "unixy.h"
#include "mime.h"
#include "superpoll.h"
#include "setting.h"
#include "version.h"
#include "control.h"
#include "log.h"
#include "logrotate.h"
#include "register.h"

extern int RUNNING;
extern uint32_t THE_CURRENT_TIME_IS;
int RELOAD = 0;

const int TICKER_TASK_STACK = 16 * 1024;
const int RELOAD_TASK_STACK = 100 * 1024;

struct ServerTask {
    bstring db_file;
    bstring server_id;
};

struct tagbstring PRIV_DIR = bsStatic("/");

Task *RELOAD_TASK = NULL;


void terminate(int s)
{
    int murder = 0;

    switch(s)
    {
        case SIGHUP:
            if(!RELOAD) {
                RELOAD = 1;
                if(RELOAD_TASK) {
                    tasksignal(RELOAD_TASK, s);
                }
            }
            break;
        default:
            if(!RUNNING) {
                log_info("SIGINT CAUGHT AGAIN, ASSUMING MURDER.");
                murder = 1;
            }
            RUNNING = 0;
            log_info("SHUTDOWN REQUESTED: %s", murder ? "MURDER" : "GRACEFUL (SIGINT again to EXIT NOW)");
            if(murder) {
                exit(s);
            }
            Server *srv = Server_queue_latest();

            if(srv != NULL) {
                shutdown(srv->listen_fd,SHUT_RDWR);
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


Server *load_server(const char *db_file, const char *server_uuid, Server *old_srv)
{
    int rc = 0;
    Server *srv =  NULL;

    rc = Config_init_db(db_file);
    check(rc == 0, "Failed to load config database at %s", db_file);
    
    rc = Config_load_settings();
    check(rc != -1, "Failed to load global settings.");

    rc = Config_load_mimetypes();
    check(rc != -1, "Failed to load mime types.");

    srv = Config_load_server(server_uuid);
    check(srv, "Failed to load server %s from %s", server_uuid, db_file);
    check(srv->default_host, "No default_host set for server: %s, you need one host named: %s", server_uuid, bdata(srv->default_hostname));

    if(old_srv == NULL || old_srv->listen_fd == -1) {
        srv->listen_fd = netannounce(TCP, bdata(srv->bind_addr), srv->port);
        check(srv->listen_fd >= 0, "Can't announce on TCP port %d", srv->port);
        check(fdnoblock(srv->listen_fd) == 0, "Failed to set listening port %d nonblocking.", srv->port);
    } else {
        srv->listen_fd = old_srv->listen_fd;
    }

    check(Server_start_handlers(srv, old_srv) == 0, "Failed to start handlers.");

    Config_close_db();
    return srv;
error:

    Server_destroy(srv);
    Config_close_db();
    return NULL;
}


int clear_pid_file(Server *srv)
{
    bstring pid_file;
    if(srv->chroot != NULL) {
        pid_file = bformat("%s%s", bdata(srv->chroot), bdata(srv->pid_file));
    } else {
        pid_file = bstrcpy(srv->pid_file);
    }

    int rc = Unixy_remove_dead_pidfile(pid_file);
    check(rc == 0, "Failed to remove the dead PID file: %s", bdata(pid_file));
    bdestroy(pid_file);

    return 0;
error:
    return -1;
}

void tickertask(void *v)
{
    (void)v;

    taskname("ticker");

    while(!task_was_signaled()) {
        THE_CURRENT_TIME_IS = time(NULL);

        int min_wait = Setting_get_int("limits.tick_timer", 10);
        taskdelay(min_wait * 1000);

        // avoid doing this during a reload attempt
        if(!RELOAD) {
            // don't bother if these are all 0
            int min_ping = Setting_get_int("limits.min_ping", DEFAULT_MIN_PING);
            int min_write_rate = Setting_get_int("limits.min_write_rate", DEFAULT_MIN_READ_RATE);
            int min_read_rate = Setting_get_int("limits.min_read_rate", DEFAULT_MIN_WRITE_RATE);

            if(min_ping > 0 || min_write_rate > 0 || min_read_rate > 0) {
                int cleared = Register_cleanout();

                if(cleared > 0) {
                    log_warn("Timeout task killed %d tasks, waiting %d seconds for more.", cleared, min_wait);
                } else {
                    debug("No connections timed out.");
                }
            }

            // do a server queue cleanup to get rid of dead servers
            Server_queue_cleanup();
        }
    }
}


int attempt_chroot_drop(Server *srv)
{
    int rc = 0;

    log_info("All loaded up, time to turn into a server.");
    log_info("-- Starting " VERSION ". Copyright (C) Zed A. Shaw. Licensed BSD.\n");
    log_info("-- Look in %s for startup messages and errors.", bdata(srv->error_log));

    int testmode = 0;

    if(srv->chroot != NULL) {
        if(Unixy_chroot(srv->chroot) == 0) {
            if(Setting_get_int("server.daemonize", 1)) {
                rc = Unixy_daemonize(1); // 1 == chdir /
                check(rc == 0, "Failed to daemonize, looks like you're hosed.");
            }
            else {
                rc = chdir("/");
                check(rc == 0, "Failed to change working directory to '/'.");
            }
        } else {
            log_warn("Couldn't chroot to %s, assuming running in test mode.", bdata(srv->chroot));

            // rewrite the access log to be in the right location
            bstring temp = bformat("%s%s", bdata(srv->chroot), bdata(srv->access_log));
            bassign(srv->access_log, temp);
            bdestroy(temp);

            temp = bformat(".%s", bdata(srv->pid_file));
            bassign(srv->pid_file, temp);
            bdestroy(temp);

            testmode = 1;
        }
    } else {
        if(Setting_get_int("server.daemonize", 1)) {
            rc = Unixy_daemonize(0); // 0 == don't chdir
            check(rc == 0, "Failed to daemonize, looks like you're hosed.");
        }
    }

    rc = Unixy_pid_file(srv->pid_file);
    check(rc == 0, "Failed to make the PID file %s", bdata(srv->pid_file));

    if(srv->chroot != NULL && ! testmode) {
        rc = Unixy_drop_priv(&PRIV_DIR);
        check(rc == 0, "Failed to drop priv to the owner of %s", bdata(&PRIV_DIR));
    } else {
        rc = Unixy_drop_priv(NULL);
        check(rc == 0, "Failed to drop priv");
    }

    if(!testmode) {
        FILE *log = fopen(bdata(srv->error_log), "a+");
        check(log, "Couldn't open %s log file.", bdata(srv->error_log));
        setbuf(log, NULL);
        check(0==add_log_to_rotate_list(srv->error_log,log),"Unable to add error log to list of files to rotate.");

        dbg_set_log(log);
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
    Server *srv = Server_queue_latest();
    Log_init(bstrcpy(srv->access_log), end_point);
    Control_port_start();
    taskdelay(500);
    log_info("-- " VERSION " Running. Copyright (C) Zed A. Shaw. Licensed BSD.");
}



void complete_shutdown(Server *srv)
{
    fdclose(srv->listen_fd);
    int attempts = 0;
    int rc = 0;
    
    rc = taskallsignal(SIGTERM);
    check(rc != -1, "Failed to send the TERM signal to all internal tasks.");

    log_info("Shutting down all running tasks as gracefully as possible.");
    
    // we will always be the last task, so wait until only 1 is running, us
    for(attempts = 0; tasksrunning() > 1 && attempts < 20; attempts++) {
        rc = taskallsignal(SIGTERM);
        check(rc != -1, "Failed to send the TERM signal to internal tasks on attempt: %d.", attempts);
    }

    log_info("Tasks now running (including main task): %d", tasksrunning());

    Control_port_stop();
    rc = Log_term();
    check(rc != -1, "Failed to shutdown the logging subsystem.");

    Setting_destroy();
    MIME_destroy();

    if(access((char *)srv->pid_file->data, F_OK) == 0) {
        log_info("Removing pid file %s", bdata(srv->pid_file));
        rc = unlink((const char *)srv->pid_file->data);
        check(rc != -1, "Failed to unlink pid_file: %s", bdata(srv->pid_file));
    }

    rc = Server_queue_destroy();
    check(rc == 0, "Failed cleaning up the server run queue.");

    Register_destroy();
    fdshutdown();

    taskexitall(0);
error:
    taskexitall(1);
}


void reload_task(void *data)
{
    RELOAD_TASK = taskself();
    (void)data; //struct ServerTask *srv = data;

    while(1) {
        taskswitch();
        task_clear_signal();

        if(RELOAD) {
            log_info("Rotating logs");
            if(rotate_logs()) {
                log_err("Error rotating logs!");
            }
            log_info("Flushing SNI cache");
            Connection_flush_sni_cache();
            RELOAD = 0;
        } else {
            log_info("Shutdown requested, goodbye.");
            break;
        }
    }

    taskexit(0);
//error:
//    taskexit(1);
}

void taskmain(int argc, char **argv)
{
    dbg_set_log(stderr);
    int rc = 0;

    check(argc == 3 || argc == 4, "usage: %s config.sqlite server_uuid [config_module.so]", m2program);

    if(argc == 4) {
        log_info("Using configuration module %s to load configs.", argv[3]);
        rc = Config_module_load(argv[3]);
        check(rc != -1, "Failed to load the config module: %s", argv[3]);
    }

    Server_queue_init();

    Server *srv = load_server(argv[1], argv[2], NULL);
    check(srv != NULL, "Aborting since can't load server.");
    Server_queue_push(srv);

    SuperPoll_get_max_fd();

    rc = clear_pid_file(srv);
    check(rc == 0, "PID file failure, aborting rather than trying to start.");

    rc = attempt_chroot_drop(srv);
    check(rc == 0, "Major failure in chroot/droppriv, aborting."); 

    // set up rng after chroot
    // TODO: once mbedtls is updated, we can move this back into Server_create
    if(srv->use_ssl) {
        rc = Server_init_rng(srv);
        check(rc == 0, "Failed to initialize rng for server %s", bdata(srv->uuid));
    }

    final_setup();

    taskcreate(tickertask, NULL, TICKER_TASK_STACK);

    struct ServerTask *srv_data = calloc(1, sizeof(struct ServerTask));
    srv_data->db_file = bfromcstr(argv[1]);
    srv_data->server_id = bfromcstr(argv[2]);

    taskcreate(reload_task, srv_data, RELOAD_TASK_STACK);

    rc = Server_run();
    check(rc != -1, "Server had a failure and exited early.");
    log_info("Server run exited, goodbye.");

    srv = Server_queue_latest();
    complete_shutdown(srv);

    return;

error:
    log_err("Exiting due to error.");
    taskexitall(1);
}

