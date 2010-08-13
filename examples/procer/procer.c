#include <unixy.h>
#include <dbg.h>
#include "procer.h"
#include <glob.h>
#include <adt/tst.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

FILE *LOG_FILE = NULL;

extern char **environ;

static inline void hardsleep(int sec)
{
    taskyield();
    sleep(sec);
}


static inline void redirect_output(const char *run_log)
{
    freopen(run_log, "a+", stdout);
    setbuf(stdout, NULL);
    freopen(run_log, "a+", stderr);
    setbuf(stdout, NULL);
    freopen("/dev/null", "r", stdin);
}


int Action_exec(Action *action, Profile *prof)
{
    int rc = 0;

    debug("ACTION: command=%s, pid_file=%s, restart=%d, depends=%s",
            bdata(prof->command), bdata(prof->pid_file), prof->restart,
            bdata(action->depends));

    pid_t pid = fork();
    check(pid >= 0, "Fork failed, WTF.  How can fork fail?");

    if(pid == 0) {
        rc = Unixy_drop_priv(action->profile_dir);

        if(rc != 0) {
            log_err("Not fatal, but we couldn't drop priv for %s",
                    bdata(action->name));
        }

        redirect_output("run.log");

        rc = execle(bdata(prof->command), bdata(prof->command), NULL, environ);
        check(rc != -1, "Failed to exec command: %s", bdata(prof->command));
    } else {
        int status = 0;
        debug("WAITING FOR CHILD.");
        pid = waitpid(pid, &status, 0);
    }

    debug("Command ran and exited successfully, now looking for the PID file.");
    return 0;
error:
    return -1;
}


void Action_task(void *v)
{
    Action *action = (Action *)v;
    int rc = 0;
    pid_t child = 0;
    Profile *prof = Profile_load(action->profile_dir);

    taskname(bdata(action->name));

    taskstate("depends");
    rc = Rampart_wait(action->before);
    check(rc != -1, "A dependency failed to start, we can't start.");

    Rampart_running(&action->after);

    taskstate("ready");
    debug("STARTED %s", bdata(action->name));

    while(1) {
        taskstate("starting");

        if(Unixy_still_running(prof->pid_file, &child)) {
            debug("Looks like %s is already running, we'll just leave it alone.", bdata(action->name));
        } else {
            Unixy_remove_dead_pidfile(prof->pid_file);
            Action_exec(action, prof);
        }

        check(access(bdata(prof->pid_file), R_OK) == 0, "%s didn't make pidfile %s.", 
                bdata(action->name), bdata(prof->pid_file));

        taskstate("waiting");
        while(Unixy_still_running(prof->pid_file, &child)) {
            hardsleep(1);
        }

        if(!prof->restart) {
            break;
        }

        taskstate("restarting");
        hardsleep(1);
    }

    debug("ACTION %s exited.", bdata(action->name));

error:
    Rampart_failed(&action->after);
    return;
}


Action *Action_create(const char *profile)
{
    Action *action = calloc(sizeof(Action), 1);
    check_mem(action);

    action->profile_dir = bfromcstr(profile);
    action->name = bTail(action->profile_dir, 
            blength(action->profile_dir) -
            bstrrchr(action->profile_dir, '/') - 1);

    action->depends = Profile_read_setting(action->profile_dir, "depends");

    return action;

error:
    return NULL;
}


int Action_depends(Action *this_one, Action *needs)
{
    check(this_one->waiting_count < MAX_DEPENDS, 
            "Too many dependencies for %s, max is %d",
            bdata(this_one->name), MAX_DEPENDS);

    this_one->before[this_one->waiting_count] = &needs->after;
    this_one->waiting_count++;

    return 0;
error:
    return -1;
}


void Action_start(Action *action)
{
    taskcreate(Action_task, action, 32 * 1024);
}

void Action_dependency_assign(void *value, void *data)
{
    Action *action = (Action *)value;
    Action *target = NULL;
    tst_t *targets = (tst_t *)data;
    int i = 0;

    if(!action->depends) return;

    debug("Processed %s action depending on: %s", bdata(action->name), bdata(action->depends));

    if(action->depends) {
        struct bstrList *dep_list = bsplit(action->depends, ' ');

        for(i = 0; i < dep_list->qty; i++) {
            bstring dep = dep_list->entry[i];

            target = (Action *)tst_search(targets, bdata(dep), blength(dep));

            if(target) {
                Action_depends(action, target);
            } else {
                log_err("Could not find dependency %s has on %s.",
                        bdata(action->name), bdata(dep));
            }
        }

        bstrListDestroy(dep_list);
    }
}

void Action_start_all(void *value, void *data)
{
    Action *action = (Action *)value;
    Action_start(action);
}


void taskmain(int argc, char *argv[])
{
    LOG_FILE = stderr;
    glob_t profile_glob;
    int rc = 0;
    int i = 0;
    Action *action = NULL;
    tst_t *targets = NULL;
    bstring pid_file = NULL;

    check(argc == 3, "USAGE: procer <profile_dir> <procer_pid_file>");
    pid_file = bfromcstr(argv[2]);

    rc = Unixy_remove_dead_pidfile(pid_file);
    check(rc == 0, "Failed to remove %s, procer is probably already running.", bdata(pid_file));

    rc = Unixy_daemonize();
    check(rc == 0, "Couldn't daemonize, that's not good.");

    rc = chdir(argv[1]);
    check(rc == 0, "Couldn't change to %s profile dir.", argv[1]);

    rc = Unixy_pid_file(pid_file);
    check(rc == 0, "Failed to make the PID file: %s", bdata(pid_file));

    FILE *log = fopen("error.log", "a+");
    check(log, "Couldn't open error.log");
    setbuf(log, NULL);
    LOG_FILE = log;

    bstring dir_glob = bformat("%s/[A-Za-z0-9]*", argv[1]);
    check(dir_glob, "Couldn't make the directory glob.");

    rc = glob(bdata(dir_glob), GLOB_ERR | GLOB_ONLYDIR, NULL, &profile_glob);
    check(rc == 0, "Failed to find directories in the profiles.");

    debug("Loading %zu actions.", profile_glob.gl_pathc);
    for(i = 0; i < profile_glob.gl_pathc; i++) {
        action = Action_create(profile_glob.gl_pathv[i]);
        targets = tst_insert(targets, bdata(action->name), blength(action->name), action);
    }

    // now we setup the dependencies from the settings they've got
    tst_traverse(targets, Action_dependency_assign, targets);
    tst_traverse(targets, Action_start_all, NULL);

    taskexit(0);

error:
    taskexitall(1);
}



