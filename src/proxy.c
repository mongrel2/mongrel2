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
    return NULL;
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

inline int Proxy_read_loop(ProxyConnect *conn)
{
    int rc = 0;


    do {
        taskstate("writing");

        rc = fdsend(conn->read_fd, conn->buffer, conn->n);

        if(rc != conn->n) {
            break;
        }
        
        taskstate("reading");
    } while((conn->n = fdrecv(conn->write_fd, conn->buffer, conn->size)) > 0);

    // no matter what, we do this
    ProxyConnect_destroy(conn);

    // TODO: do some better error checking here by looking at errno
    return 0;
}

int Proxy_connect(Proxy *proxy, int fd, const char *buf, size_t len, size_t n)
{
    ProxyConnect *to_listener = NULL;
    ProxyConnect *to_proxy = NULL;

    taskname("proxy_to_listener");

    char *initial_buf = malloc(len);
    check(initial_buf, "Out of memory.");
    check(memcpy(initial_buf, buf, n), "Failed to copy the initial buffer.");

    to_proxy = ProxyConnect_create(fd, initial_buf, len, n); 

    check(to_proxy, "Could not create the connection to backend %s:%d", proxy->server, proxy->port);


    debug("Connecting to %s:%d", proxy->server, proxy->port);

    // TODO: create release style macros that compile these away
    taskstate("connecting");

    to_proxy->read_fd = netdial(TCP, proxy->server, proxy->port);
    check(to_proxy->read_fd >= 0, "Failed to connect to %s:%d", proxy->server, proxy->port);

    fdnoblock(to_proxy->read_fd);
    fdnoblock(to_proxy->write_fd);

    taskstate("connected");
    debug("Proxy connected to %s:%d", proxy->server, proxy->port);
    
    to_listener = ProxyConnect_create(to_proxy->read_fd, 
            malloc(to_proxy->size), to_proxy->size, 0);

    check(to_listener, "Could not create the connection to backend %s:%d", proxy->server, proxy->port);
    
    to_listener->read_fd = to_proxy->write_fd;

    assert(to_listener->write_fd != to_proxy->write_fd && "Wrong write fd setup.");
    assert(to_listener->read_fd != to_proxy->read_fd && "Wrong read fd setup.");


    // kick off one side as a task to do its thing
    taskcreate(rwtask, (void *)to_proxy, STACK);

    // rather than spawn a whole new task for this side, we just call our loop
    return Proxy_read_loop(to_listener);

error:
    ProxyConnect_destroy(to_proxy);
    ProxyConnect_destroy(to_listener);
    return -1;
}


void
rwtask(void *v)
{
    taskname("proxy_to_proxy");
    ProxyConnect *conn = (ProxyConnect *)v;
    // return value ignored since this is a task
    Proxy_read_loop(conn);
}

