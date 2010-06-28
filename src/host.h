#ifndef _host_h
#define _host_h

#include <adt/tst.h>
#include <proxy.h>
#include <handler.h>
#include <dir.h>
#include <routing.h>

enum {
   MAX_HOST_NAME=256,
   MAX_URL_PATH=256
};


typedef struct Host {
    RouteMap *routes;
    char name[MAX_HOST_NAME];
} Host;


typedef enum BackendType {
    BACKEND_HANDLER=1, BACKEND_PROXY, BACKEND_DIR
} BackendType;

typedef struct Backend {
    int type;

    union {
        Handler *handler;
        Proxy *proxy;
        Dir *dir;
    } target;
} Backend;

Host *Host_create(const char *name);
void Host_destroy(Host *host);

int Host_add_backend(Host *host, const char *path, size_t path_len, BackendType type, void *target);


Backend *Host_match(Host *host, const char *target, size_t len);

#endif
