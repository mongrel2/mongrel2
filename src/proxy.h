#ifndef _proxy_h
#define _proxy_h

typedef struct ProxyConnect {
    int write_fd;
    int read_fd;
    char *buffer;
    size_t size;
    size_t n;
} ProxyConnect;

ProxyConnect *ProxyConnect_create(int write_fd, char *buffer, size_t size, size_t n);

void ProxyConnect_destroy(ProxyConnect *conn);

void proxy_init(char *server, int port);

void proxy_connect(ProxyConnect *conn);

#endif
