#ifndef _server_h
#define _server_h

#include <adt/tst.h>
#include <adt/list.h>
#include <host.h>
#include <routing.h>

typedef struct Server {
    int port;
    int listen_fd;

    RouteMap *hosts;
} Server;


Server *Server_create(const char *port);
void Server_destroy(Server *srv);
void Server_init();

void Server_start(Server *srv);
int Server_add_host(Server *srv, const char *pattern, size_t len, Host *host);

#endif
