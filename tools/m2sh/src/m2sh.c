#include <dbg.h>
#include <task/task.h>
#include <pattern.h>
#include "commands.h"

FILE *LOG_FILE = NULL;


void taskmain(int argc, char *argv[])
{
    LOG_FILE = stderr;
    int i = 0;

    bstring arguments = bfromcstr(argv[0]);

    for(i = 1; i < argc; i++) {
        bcatcstr(arguments, " ");
        bcatcstr(arguments, argv[i]);
    }

    taskexitall(Command_run(arguments));
}

