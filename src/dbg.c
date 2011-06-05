#include "dbg.h"

FILE *LOG_FILE = NULL;

void dbg_set_log(FILE *log_file)
{
    LOG_FILE = log_file;
}


FILE *dbg_get_log()
{
    return LOG_FILE != NULL ? LOG_FILE : stderr;
}
