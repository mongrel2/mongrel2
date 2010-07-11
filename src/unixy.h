#ifndef _unixy_h
#define _unixy_h

#include <bstring.h>

int Unixy_chroot(bstring path);

int Unixy_drop_priv(bstring path);

bstring Unixy_getcwd();

int Unixy_pid_file(bstring path);

int Unixy_daemonize();

int Unixy_still_running(bstring pid_path, pid_t *pid);

int Unixy_remove_dead_pidfile(bstring pid_path);

pid_t Unixy_pid_read(bstring pid_path);

#endif
