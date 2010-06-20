#ifndef _host_h
#define _host_h

#define MAX_HOST_NAME 256
#define MAX_URL_PATH 256

#include <adt/tst.h>
#include <handler.h>
#include <proxy.h>

typedef struct Host {
    tst_t *routes;
    char name[MAX_HOST_NAME];
} Host;


typedef enum RouteType {
    HANDLER=1, PROXY, LAST
} RouteType;

typedef struct Route {
    int type;

    union {
        Handler *handler;
        Proxy *proxy;
    } target;

    char path[MAX_URL_PATH];
} Route;

Host *Host_create(const char *name);
void Host_destroy(Host *host);

int Host_add_route(const char *path, RouteType type, void *target);


#endif
