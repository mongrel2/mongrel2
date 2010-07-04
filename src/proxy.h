#ifndef _proxy_h
#define _proxy_h

#include <bstring.h>
#include <task/task.h>

typedef struct ProxyConnect {
    int client_fd;
    int proxy_fd;
    char *buffer;
    size_t size;
    int n;
    Rendez *waiter;
} ProxyConnect;

typedef struct Proxy {
    bstring server;
    int port;
} Proxy;

ProxyConnect *ProxyConnect_create(int client_fd, char *buffer, size_t size, size_t n);

void ProxyConnect_destroy(ProxyConnect *conn);

Proxy *Proxy_create(bstring server, int port);

void Proxy_destroy(Proxy *proxy);

ProxyConnect *Proxy_connect_backend(Proxy *proxy, int fd, const char *buf, 
        size_t len, size_t nread);

ProxyConnect *Proxy_sync_to_listener(ProxyConnect *to_proxy);
#endif
