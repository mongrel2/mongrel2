#include <dbg.h>
#include <task/task.h>
#include <proxy.h>
#include <listener.h>
#include <register.h>
#include <handler.h>
#include <server.h>
#include <host.h>
#include <assert.h>



Server *Server_create(const char *port)
{
    Server *srv = calloc(sizeof(Server), 1);
    check(srv, "Out of memory.");

    srv->handlers = list_create(LISTCOUNT_T_MAX);
    check(srv->handlers, "Can't create handlers list.");

    srv->proxies = list_create(LISTCOUNT_T_MAX);
    check(srv->proxies, "Can't create proxies list.");

    srv->hosts = NULL;

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
        // TODO: leaks the actual handlers and proxies
        list_destroy_nodes(srv->proxies);
        list_destroy(srv->proxies);
        list_destroy_nodes(srv->handlers);
        list_destroy(srv->handlers);
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
}


void Server_start(Server *srv)
{
    int cfd;
    int rport;
    char remote[16];

    check(!(list_isempty(srv->proxies) || list_isempty(srv->handlers)), "No proxies or handlers created, you need something.");

    Handler *handler = (Handler *)lnode_get(list_first(srv->handlers));
    Proxy *proxy = (Proxy *)lnode_get(list_first(srv->proxies));

    debug("Starting server on port %d", srv->port);
    taskcreate(Handler_task, handler, HANDLER_STACK);

    while((cfd = netaccept(srv->listen_fd, remote, &rport)) >= 0) {
        Listener_accept(handler, proxy, cfd, remote);
    }

    return;

error:
    taskexitall(1);
}

int Server_add_proxy(Server *srv, Proxy *proxy)
{
    check(!list_isfull(srv->proxies), "Too many proxies.");

    list_append(srv->proxies, lnode_create(proxy));

    return 1;

error:
    return -1;
}


int Server_add_handler(Server *srv, Handler *handler)
{
    check(!list_isfull(srv->handlers), "Too many handlers.");

    list_append(srv->handlers, lnode_create(handler));

    return 1;
error:

    return -1;
}


int Server_add_host(Server *srv, Host *host)
{
    srv->hosts = tst_insert(srv->hosts, host->name, sizeof(host->name), host);

    assert(srv->hosts && "Failed to insert host.");
}


