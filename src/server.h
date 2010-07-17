#ifndef _server_h
#define _server_h

#include <adt/tst.h>
#include <adt/list.h>
#include <host.h>
#include <routing.h>

enum {
    IPADDR_SIZE = 16
};

typedef struct Server {
    int port;
    int listen_fd;
    Host *default_host;
    RouteMap *hosts;
    bstring uuid;
    bstring chroot;
    bstring access_log;
    bstring error_log;
    bstring pid_file;
} Server;


Server *Server_create(const char *uuid, const char *port,
        const char *chroot, const char *access_log,
        const char *error_log, const char *pid_file);

void Server_destroy(Server *srv);

void Server_init();

void Server_start(Server *srv);

int Server_add_host(Server *srv, bstring pattern, Host *host);

void Server_set_default_host(Server *srv, Host *host);

Host *Server_match_backend(Server *srv, bstring target);

#endif
