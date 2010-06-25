#include <server.h>
#include <dbg.h>
#include <task/task.h>
#include <string.h>
#include <config/config.h>
#include <adt/list.h>

FILE *LOG_FILE = NULL;

void taskmain(int argc, char **argv)
{
    LOG_FILE = stderr;

    int rc = 0;
    Server *srv = NULL;

    check(argc == 2, "usage: server config.sqlite");

    Server_init();

    list_t *servers = Config_load(argv[1]);
    check(list_count(servers) == 1, "Currently only support running one server.");

    srv = lnode_get(list_first(servers));

    Server_start(srv);

    return;

error:
    log_err("Exiting due to error.");
    taskexitall(1);
}

