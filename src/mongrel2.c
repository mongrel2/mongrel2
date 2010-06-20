#include <server.h>
#include <dbg.h>
#include <task/task.h>

FILE *LOG_FILE = NULL;

char *UUID = "907F620B-BC91-4C93-86EF-512B71C2AE27";


void taskmain(int argc, char **argv)
{
    LOG_FILE = stderr;

    int rc = 0;
    Server *srv = NULL;

    check(argc == 4, "usage: server localport handlerq listenerq");
    char *send_spec = argv[2];
    char *recv_spec = argv[3];

    Server_init();

    srv = Server_create(argv[1]);
    check(srv, "Failed to create server.");
    
    rc = Server_add_proxy(srv, Proxy_create("127.0.0.1", 80));
    check(rc != -1, "Failed to add proxy to server config.");

    rc = Server_add_handler(srv, Handler_create(send_spec, UUID, recv_spec, ""));
    check(rc != -1, "Failed to add handler to server config.");

    Server_start(srv);

    return;

error:
    log_err("Exiting due to error.");
    taskexitall(1);
}

