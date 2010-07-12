#include <unixy.h>
#include <dbg.h>
#include <dir.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>



int Unixy_chroot(bstring path)
{
    int rc = 0;
    const char *to_dir = bdata(path);

    check(to_dir && blength(path) > 0, "Invalid or empty path for chroot.");

    rc = chroot(to_dir);
    check(rc == 0, "Can't chroot to %s, rerun as root.", bdata(path));

    return 0;

error:
    return -1;
}


int Unixy_drop_priv(bstring path)
{
    const char *from_dir = bdata(path);
    struct stat sb;

    check(from_dir && blength(path), "Chroot path can't be empty.");

    int rc = stat(from_dir, &sb);
    check(rc == 0, "Failed to stat target chroot directory: %s", bdata(path));

    rc = setgid(sb.st_gid);
    check(rc == 0, "Failed to change to GID: %d", sb.st_gid);

    rc = setuid(sb.st_uid);
    check(rc == 0, "Failed to change to UID: %d", sb.st_uid);

    debug("Now running as UID:%d, GID:%d", sb.st_uid, sb.st_gid);
    return 0;

error:
    return -1;
}


pid_t Unixy_pid_read(bstring pid_path)
{
    pid_t pid = -1;
    FILE *pid_file = NULL;

    check(pid_path, "PID file has not been set yet.");

    pid_file = fopen(bdata(pid_path), "r");
    check(pid_file, "Failed to open PID file %s for reading.", bdata(pid_path));

    fscanf(pid_file, "%d", &pid);

    fclose(pid_file);
    return pid;
error:

    if(pid_file) fclose(pid_file);
    return -1;
}

int Unixy_still_running(bstring pid_path, pid_t *pid)
{
    bstring proc_file = NULL;
    struct stat sb;
    int rc = 0;
    FILE *proc_test = NULL;

    *pid = Unixy_pid_read(pid_path);

    if(*pid < 0) {
        return 0;
    }
    
    // If we can signal it, it must be alive.
    rc = kill(*pid, 0);

    bdestroy(proc_file);
    return rc == 0;
error:

    if(proc_test) fclose(proc_test);
    bdestroy(proc_file);
    return -1;
}


int Unixy_remove_dead_pidfile(bstring pid_path)
{
    int rc = 0;
    pid_t pid = 0;

    rc = Unixy_still_running(pid_path, &pid);

    check(rc != -1, "Failed to figure out if process %d is still running.  You probably don't have /proc or a weird one.", pid);
    check(rc != 1, "Process %d is still running, shut it down first.", pid);

    if(pid == -1) {
        debug("No previous mongrel running, continuing on.");
    } else {
        debug("Process %d is not running anymore, so removing PID file %s.", pid, bdata(pid_path));
        rc = unlink((const char *)pid_path->data);
        check(rc == 0, "Failed to unline the PID file %s, man you're so hosed I give up.", bdata(pid_path));
    }

    return 0;

error:
    return -1;
}

int Unixy_pid_file(bstring path)
{
    FILE *pid_file = NULL;
    int rc = 0;
    struct stat sb;
    char *pid_path = NULL;

    pid_path = bstr2cstr(path, '\0');
    check(pid_path, "Failed to make the pid path (WTF).");

    rc = stat(pid_path, &sb);
    check(rc == -1, "PID file already exists, something bad happened.");

    // pid file isn't there, open it and make it
    pid_file = fopen(pid_path, "w");
    check(pid_file, "Failed to open PID file %s for writing.", pid_path);

    rc =  fprintf(pid_file, "%d", getpid());
    check(rc > 0, "Failed to write PID to file %s", pid_path); 
   

    fclose(pid_file);
    return 0;

error:
    if(pid_path) {
        free(pid_path);
        pid_path = NULL;
    }

    if(pid_file) fclose(pid_file);
    return -1;
}


int Unixy_daemonize()
{
    // daemonize is just too damn eager on closing stuff
    int rc = daemon(0, 1);
    check(rc == 0, "Failed to daemonize.");

    return 0;

error:
    return -1;
}


bstring Unixy_getcwd()
{
    char wd[MAX_DIR_PATH];

    check(getcwd(wd, MAX_DIR_PATH-1), "Could not get current working directory.");
    wd[MAX_DIR_PATH] = '\0';

    return bfromcstr(wd);

error:
    return NULL;
}

