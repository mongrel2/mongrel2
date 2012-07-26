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

#include <unixy.h>
#include <dbg.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <limits.h>
#include <stdlib.h>

char *m2program = "mongrel2";

int Unixy_chroot(bstring path)
{
    int rc = 0;
    const char *to_dir = bdata(path);

    check(to_dir && blength(path) > 0, "Invalid or empty path for chroot.");

    rc = chroot(to_dir);
    check(rc == 0, "Can't chroot to %s, rerun as root if this is what you want.", bdata(path));

    rc = chdir("/");
    check(rc == 0, "Can't chdir to / directory inside chroot.");

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

    rc = setregid(sb.st_gid, sb.st_gid);
    check(rc == 0 && getgid() == sb.st_gid && getegid() == sb.st_gid, "Failed to change to GID: %d", sb.st_gid);

    rc = setreuid(sb.st_uid, sb.st_uid);
    check(rc == 0 && getuid() == sb.st_uid && geteuid() == sb.st_uid, "Failed to change to UID: %d", sb.st_uid);

    log_info("Now running as UID:%d, GID:%d", sb.st_uid, sb.st_gid);
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

    check(fscanf(pid_file, "%d", &pid) != -1, "Failed to read an integer from PID file: %s", bdata(pid_path));

    fclose(pid_file);
    return pid;
error:

    if(pid_file) fclose(pid_file);
    return -1;
}

int Unixy_still_running(bstring pid_path, pid_t *pid)
{
    int rc = 0;

    *pid = Unixy_pid_read(pid_path);

    if(*pid < 0) {
        return 0;
    }
    
    // If we can signal it, it must be alive.
    rc = kill(*pid, 0);
    return rc == 0;
}


int Unixy_remove_dead_pidfile(bstring pid_path)
{
    int rc = 0;
    pid_t pid = 0;

    rc = Unixy_still_running(pid_path, &pid);

    check(rc != 1, "Process %d is still running, shut it down first.", pid);

    if(pid == -1) {
        log_info("No previous %s running, continuing on.", m2program);
    } else {
        log_info("Process %d is not running anymore, so removing PID file %s.", pid, bdata(pid_path));
        rc = unlink((const char *)pid_path->data);
        check(rc == 0, "Failed to remove the PID file %s, man you're so hosed I give up.", bdata(pid_path));
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
    check_mem(pid_path);

    rc = stat(pid_path, &sb);
    check(rc == -1, "PID file already exists, something bad happened.");

    // pid file isn't there, open it and make it
    pid_file = fopen(pid_path, "w");
    check(pid_file, "Failed to open PID file %s for writing.", pid_path);

    rc =  fprintf(pid_file, "%d", getpid());
    check(rc > 0, "Failed to write PID to file %s", pid_path); 
   

    if(pid_path) free(pid_path);
    fclose(pid_file);
    return 0;

error:
    if(pid_path) free(pid_path);
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
    char *wd = calloc(PATH_MAX + 1, 1);
    bstring dir = NULL;

    check(getcwd(wd, PATH_MAX-1), "Could not get current working directory.");
    wd[PATH_MAX] = '\0';

    dir = bfromcstr(wd);

error:
    // fall through
    free(wd);
    return dir;
}

