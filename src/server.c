#include <dbg.h>
#include <task/task.h>
#include <proxy.h>
#include <listener.h>
#include <register.h>
#include <handler.h>
#include <server.h>


FILE *LOG_FILE = NULL;

char *UUID = "907F620B-BC91-4C93-86EF-512B71C2AE27";


Server *Server_create(const char *port)
{
    Server *srv = calloc(sizeof(Server), 1);
    check(srv, "Out of memory.");

    srv->port = atoi(port);
    check(port > 0, "Can't bind to the given port: %s", port);

    srv->listen_fd = netannounce(TCP, 0, srv->port);
    check(srv->listen_fd >= 0, "Can't announce on TCP port %d", srv->port);

    check(fdnoblock(srv->listen_fd) == 0, "Failed to set listening port %d nonblocking.", srv->port);

    return srv;

error:
    Server_destroy(srv);
    return NULL;
}

void Server_destroy(Server *srv)
{
    if(srv) {
        Handler_destroy(srv->handler, 0);
        Proxy_destroy(srv->proxy);
        close(srv->listen_fd);
        free(srv);
    }
}


void Server_init()
{
    mqinit(2);
    Register_init();
    Listener_init();
    Handler_init();

    LOG_FILE = stderr;
}


void Server_start(Server *srv)
{
    int cfd;
    int rport;
    char remote[16];

    debug("Starting server on port %d", srv->port);
    taskcreate(Handler_task, srv->handler, HANDLER_STACK);

    while((cfd = netaccept(srv->listen_fd, remote, &rport)) >= 0) {
        Listener_accept(srv->handler, srv->proxy, cfd, remote);
    }
}

int Server_add_proxy(Server *srv, Proxy *proxy)
{
    check(srv->proxy == NULL, "Too many proxies.");

    srv->proxy = proxy;
    check(srv->proxy, "Failed to create proxy configuration.");

    return 1;

error:
    return -1;
}


int Server_add_handler(Server *srv, Handler *handler)
{
    check(srv->handler == NULL, "Too many handlers.");

    srv->handler = handler;
    check(srv->handler, "Handler is invalid.");

    return 1;
error:

    return -1;
}


void taskmain(int argc, char **argv)
{
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

