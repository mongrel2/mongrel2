#include <server.h>
#include <dbg.h>
#include <task/task.h>
#include <string.h>
#include <config/config.h>
#include <adt/list.h>
#include <config/db.h>
#include <unixy.h>


FILE *LOG_FILE = NULL;

struct tagbstring PRIV_DIR = bsStatic("/");

void taskmain(int argc, char **argv)
{
    LOG_FILE = stderr;
    int rc = 0;

    check(argc == 3, "usage: server config.sqlite default_host");

    list_t *servers = Config_load_servers(argv[1], argv[2]);

    check(servers, "Failed to load server config from %s for host %s", argv[1], argv[2]);
    check(list_count(servers) == 1, "Currently only support running one server.");

    rc = Config_load_mimetypes();
    check(rc == 0, "Failed to load mime types.");

    Server *srv = lnode_get(list_first(servers));

    DB_close();


    bstring pid_file = bformat("%s%s", bdata(srv->chroot), bdata(srv->pid_file));

    rc = Unixy_remove_dead_pidfile(pid_file);
    check(rc == 0, "Failed to remove the dead PID file: %s", bdata(pid_file));
    bdestroy(pid_file);

    rc = Unixy_chroot(srv->chroot);

    if(rc == 0) {
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

    Server_init();
    Server_start(srv);

    return;

error:
    log_err("Exiting due to error.");
    taskexitall(1);
}

