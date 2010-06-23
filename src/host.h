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


typedef enum BackendType {
    BACKEND_HANDLER, BACKEND_PROXY
} BackendType;

typedef struct Backend {
    int type;

    union {
        Handler *handler;
        Proxy *proxy;
    } target;
} Backend;

Host *Host_create(const char *name);
void Host_destroy(Host *host);

int Host_add_backend(Host *host, const char *path, size_t path_len, BackendType type, void *target);


#endif
