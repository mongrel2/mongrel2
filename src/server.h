#ifndef _server_h
#define _server_h

#include <adt/tst.h>
#include <adt/hash.h>
#include <handler.h>

typedef struct Server {
    int port;
    int listen_fd;

    Handler *handler;
    Proxy *proxy;
} Server;


Server *Server_create(const char *port);
void Server_destroy(Server *srv);
void Server_init();

void Server_start(Server *srv);
int Server_add_handler(Server *srv, Handler *handler);
int Server_add_proxy(Server *srv, Proxy *proxy);

#endif
