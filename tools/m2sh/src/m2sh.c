#include <dbg.h>
#include <task/task.h>
#include <pattern.h>
#include "config_file.h"

FILE *LOG_FILE = NULL;

void taskmain(int argc, char *argv[])
{
    LOG_FILE = stderr;
    if(argc != 3) {
        log_err("USAGE: m2sh config.conf config.sqlite");
        taskexitall(1);
    } else {
        Config_load(argv[1], argv[2]);
        taskexitall(0);
    }
}

