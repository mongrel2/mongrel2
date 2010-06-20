#ifndef _server_h
#define _server_h

#include <adt/tst.h>
#include <adt/list.h>
#include <handler.h>
#include <host.h>

typedef struct Server {
    int port;
    int listen_fd;

    list_t *handlers;
    list_t *proxies;
    tst_t *hosts;
} Server;


Server *Server_create(const char *port);
void Server_destroy(Server *srv);
void Server_init();

void Server_start(Server *srv);
int Server_add_handler(Server *srv, Handler *handler);
int Server_add_proxy(Server *srv, Proxy *proxy);
int Server_add_host(Server *srv, Host *host);

#endif
