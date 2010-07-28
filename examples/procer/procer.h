#ifndef _procer_h
#define _procer_h

#include <bstring.h>
#include <task/task.h>
#include <stdio.h>
#include <stdlib.h>

enum {
    MAX_DEPENDS = 128
};

typedef struct Rampart {
    Rendez wait;
    int running;
    int failed;
} Rampart;


typedef struct Profile {
    // this gets run
    bstring command;
    // this is the pidfile to check
    bstring pid_file;
    // whether to restart it or not
    int restart;
} Profile;



typedef struct Action {
    // the action's name for depends and logging
    bstring name;
    // where the action will load from
    bstring profile_dir;
    // what other stuff this depends on
    bstring depends;

    // and each task wakes all on after once it's running so children wake
    Rampart after;

    int waiting_count;

    // each task sleeps on before so that it waits for it's depends
    Rampart *before[MAX_DEPENDS];
} Action;


#define Rampart_should_wait(R) !((R)->running || (R)->failed)

int Rampart_wait(Rampart *on[]);

void Rampart_running(Rampart *on);

void Rampart_notrunning(Rampart *on);

void Rampart_failed(Rampart *on);

bstring Profile_read_setting(bstring path, const char *file);

int Profile_check_setting(bstring path, const char *file);

Profile *Profile_load(bstring profile_dir);

#endif
