#include <dbg.h>
#include <task/task.h>
#include <bstring.h>
#include <pattern.h>
#include <getopt.h>

FILE *LOG_FILE = NULL;

void taskmain(int argc, char *argv[])
{
    LOG_FILE = stderr;
    debug("argc: %d, argv: %p", argc, argv);

    taskexitall(0);

}
