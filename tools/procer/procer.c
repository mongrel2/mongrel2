#include <unixy.h>
#include <dbg.h>
#include "procer.h"
#include <glob.h>
#include <adt/tst.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>

extern char **environ;

static int RUNNING = 1;

void terminate(int s)
{
    switch(s) {
        case SIGHUP:
            // TODO: do a reload
            break;
        case SIGINT:
            // TODO: do a graceful stop
            RUNNING = 0;
            break;
        case SIGTERM:
            RUNNING = 0;
            break;
        default:
            break;
    }
}

static inline void Action_sleep(int sec)
{
    taskdelay(sec * (1000 + rand() % 1000));
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
    char *procer_run_log = NULL;

    bstring pidfile_env = bformat("PROCER_PIDFILE=%s", bdata(prof->pid_file)); 
    putenv(bdata(pidfile_env));

    bstring action_env = bformat("PROCER_ACTION=%s", bdata(action->name)); 
    putenv(bdata(action_env));

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

        if( (procer_run_log = getenv("PROCER_RUN_LOG")) == NULL)
            procer_run_log = "run.log";
        redirect_output(procer_run_log);

        rc = execle(bdatae(prof->command, ""), bdatae(prof->command, ""), NULL, environ);
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

    while(RUNNING) {
        taskstate("starting");

        if(Unixy_still_running(prof->pid_file, &child)) {
            debug("Looks like %s is already running, we'll just leave it alone.", bdata(action->name));
        } else {
            Unixy_remove_dead_pidfile(prof->pid_file);
            Action_exec(action, prof);
        }

        if(access(bdatae(prof->pid_file, ""), R_OK) != 0) {
            log_warn("%s didn't make pidfile %s. Waiting then trying again.", 
                bdata(action->name), bdata(prof->pid_file));
            Action_sleep(2);
        }

        taskstate("waiting");
        while(Unixy_still_running(prof->pid_file, &child) && RUNNING) {
            Action_sleep(1);
        }

        if(!prof->restart) {
            break;
        }

        taskstate("restarting");
        Action_sleep(1);
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

char *procer_program = "procer";

void taskmain(int argc, char *argv[])
{
    dbg_set_log(stderr);
    glob_t profile_glob;
    int rc = 0;
    int i = 0;
    Action *action = NULL;
    tst_t *targets = NULL;
    bstring pid_file = NULL;
    char *procer_error_log = NULL;

    m2program = procer_program;

    check(argc == 3, "USAGE: %s <profile_dir> <procer_pid_file>", m2program);
    pid_file = bfromcstr(argv[2]);

    srand(time(NULL)); // simple randomness

    rc = Unixy_remove_dead_pidfile(pid_file);
    check(rc == 0, "Failed to remove %s, %s is probably already running.", bdata(pid_file), m2program);

    rc = Unixy_daemonize(1);
    check(rc == 0, "Couldn't daemonize, that's not good.");

    rc = chdir(argv[1]);
    check(rc == 0, "Couldn't change to %s profile dir.", argv[1]);

    rc = Unixy_pid_file(pid_file);
    check(rc == 0, "Failed to make the PID file: %s", bdata(pid_file));

    if( (procer_error_log = getenv("PROCER_ERROR_LOG")) == NULL)
        procer_error_log = "error.log";
    FILE *log = fopen(procer_error_log, "a+");
    check(log, "Couldn't open error.log");
    setbuf(log, NULL);
    dbg_set_log(log);

    bstring dir_glob = bformat("%s/[A-Za-z0-9]*", argv[1]);
    check(dir_glob, "Couldn't make the directory glob.");

    rc = glob(bdata(dir_glob), GLOB_ERR, NULL, &profile_glob);
    check(rc == 0, "Failed to find directories in the profiles.");

    struct stat sb;
    debug("Loading %zu actions.", profile_glob.gl_pathc);

    start_terminator();

    while(RUNNING) {
        for(i = 0; i < profile_glob.gl_pathc; i++) {
            rc = lstat(profile_glob.gl_pathv[i], &sb);
            check(rc == 0, "Failed to stat file or directory: %s", profile_glob.gl_pathv[i]);

            if (sb.st_mode & S_IFDIR) {
                action = Action_create(profile_glob.gl_pathv[i]);
                targets = tst_insert(targets, bdata(action->name), blength(action->name), action);
            }
        }

        // now we setup the dependencies from the settings they've got
        tst_traverse(targets, Action_dependency_assign, targets);
        tst_traverse(targets, Action_start_all, NULL);

        while(RUNNING) {
            Action_sleep(1);
        }
    }

    log_warn("Exiting as requested from procer.");

    taskexit(0);

error:
    taskexitall(1);
}



