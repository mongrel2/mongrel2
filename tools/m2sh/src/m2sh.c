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
        bstring a = bfromcstr(argv[i]);

        // compensate for quotes getting taken off by the shell
        // TODO: also need to escape " to bring back that
        if(bstrchr(a, ' ') != -1) {
            bcatcstr(arguments, " \"");
            bconcat(arguments, a);
            bcatcstr(arguments, "\"");
        } else {
            bcatcstr(arguments, " ");
            bconcat(arguments, a);
        }

        bdestroy(a);
    }

    debug("RUNNING: %s", bdata(arguments));
    taskexitall(Command_run(arguments));
}

