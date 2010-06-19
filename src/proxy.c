#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <task/task.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <dbg.h>
#include <proxy.h>
#include <assert.h>
#include <stdlib.h>

enum
{
    STACK = 32768
};


void rwtask(void*);


ProxyConnect *ProxyConnect_create(int write_fd, char *buffer, size_t size, size_t n)
{
    ProxyConnect *conn = malloc(sizeof(ProxyConnect));
    check(conn, "Failed to allocate ProxyConnect.");
    conn->write_fd = write_fd;
    conn->read_fd = 0;
    conn->buffer = buffer;
    conn->size = size;
    conn->n = n; 

    return conn;
error:
    taskexitall(1);
}

void ProxyConnect_destroy(ProxyConnect *conn) 
{
    if(conn) {
        if(conn->write_fd) shutdown(conn->write_fd, SHUT_WR);
        if(conn->read_fd) close(conn->read_fd);
        if(conn->buffer) free(conn->buffer);
        free(conn);
    }
}


void Proxy_destroy(Proxy *proxy)
{
    if(proxy) {
        if(proxy->server) free(proxy->server);
        free(proxy);
    }
}

Proxy *Proxy_create(char *server, int port)
{
    Proxy *proxy = calloc(sizeof(Proxy), 1);
    check(proxy, "Failed to create proxy, memory allocation fail.");
    
    proxy->server = strdup(server);
    check(proxy->server, "Out of ram.");

    proxy->port = port;

    return proxy;

error:
    Proxy_destroy(proxy);
    return NULL;
}


void Proxy_connect(Proxy *proxy, int fd, char *buf, size_t len, size_t n)
{
    ProxyConnect *to_listener = NULL;
    ProxyConnect *to_proxy = ProxyConnect_create(fd, buf, 1024, n); 

    check(to_proxy, "Could not create the connection to backend %s:%d", proxy->server, proxy->port);


    debug("Connecting to %s:%d", proxy->server, proxy->port);

    to_proxy->read_fd = netdial(TCP, proxy->server, proxy->port);
    check(to_proxy->read_fd >= 0, "Failed to connect to %s:%d", proxy->server, proxy->port);

    fdnoblock(to_proxy->read_fd);
    fdnoblock(to_proxy->write_fd);

    debug("Proxy connected to %s:%d", proxy->server, proxy->port);
    
    to_listener = ProxyConnect_create(to_proxy->read_fd, 
            malloc(to_proxy->size), to_proxy->size, 0);

    check(to_listener, "Could not create the connection to backend %s:%d", proxy->server, proxy->port);
    
    to_listener->read_fd = to_proxy->write_fd;

    assert(to_listener->write_fd != to_proxy->write_fd && "Wrong write fd setup.");
    assert(to_listener->read_fd != to_proxy->read_fd && "Wrong read fd setup.");

    taskcreate(rwtask, (void *)to_listener, STACK);
    taskcreate(rwtask, (void *)to_proxy, STACK);

error:
    ProxyConnect_destroy(to_proxy);
    ProxyConnect_destroy(to_listener);
}


void
rwtask(void *v)
{
    ProxyConnect *conn = (ProxyConnect *)v;    
    int rc = 0;

    do {
        rc = fdwrite(conn->read_fd, conn->buffer, conn->n);
        check(rc == conn->n, "Connection closed.");
    } while((conn->n = fdread(conn->write_fd, conn->buffer, conn->size)) > 0);

error:
    ProxyConnect_destroy(conn);
}

