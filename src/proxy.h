#ifndef _proxy_h
#define _proxy_h

#include <bstring.h>

typedef struct ProxyConnect {
    int write_fd;
    int read_fd;
    char *buffer;
    size_t size;
    size_t n;
} ProxyConnect;

typedef struct Proxy {
    bstring server;
    int port;
} Proxy;

ProxyConnect *ProxyConnect_create(int write_fd, char *buffer, size_t size, size_t n);

void ProxyConnect_destroy(ProxyConnect *conn);

Proxy *Proxy_create(bstring server, int port);

void Proxy_destroy(Proxy *proxy);

int Proxy_connect(Proxy *proxy, int fd, const char *buf, size_t len, size_t n);

#endif
