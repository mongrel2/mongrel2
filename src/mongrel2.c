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
    check(argc == 3, "usage: server config.sqlite default_host");

    Server_init();

    list_t *servers = Config_load_servers(argv[1], argv[2]);
    check(servers, "Failed to load server config from %s for host %s", argv[1], argv[2]);
    check(list_count(servers) == 1, "Currently only support running one server.");

    check(Config_load_mimetypes() == 0, "Failed to load mime types.");

    Server *srv = lnode_get(list_first(servers));

    Server_start(srv);

    return;

error:
    log_err("Exiting due to error.");
    taskexitall(1);
}

