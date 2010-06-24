#include <server.h>
#include <dbg.h>
#include <task/task.h>
#include <string.h>

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

    Host *host = Host_create("mongrel2.org");
    check(host, "Couldn't create mongrel2.org host.");

    Proxy *web_server = Proxy_create("127.0.0.1", 80);
    check(web_server, "Failed to add web proxy to server config.");

    Handler *chat_svc = Handler_create(send_spec, UUID, recv_spec, "");
    check(chat_svc, "Failed to add chat service to server config.");

    rc = Host_add_backend(host, "@chat", strlen("@chat"), BACKEND_HANDLER, chat_svc);
    check(rc == 0, "Adding chat server backend failed.");

    Host_add_backend(host, "/chat/", strlen("/chat/"), BACKEND_PROXY, web_server);
    check(rc == 0, "Adding web proxy backend failed.");

    const char *host_pattern = "mongrel2.org";

    rc = Server_add_host(srv, host_pattern, strlen(host_pattern), host);
    check(rc == 0, "Could not add the main host to the server.");

    Server_set_default_host(srv, host);

    Server_start(srv);

    return;

error:
    log_err("Exiting due to error.");
    taskexitall(1);
}

