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

enum
{
    STACK = 32768
};

char *SERVER;
int PORT;

void proxytask(void*);
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

void ProxyConnect_destroy(ProxyConnect *conn) {
    if(conn->write_fd) shutdown(conn->write_fd, SHUT_WR);
    if(conn->read_fd) close(conn->read_fd);
    if(conn->buffer) free(conn->buffer);
    free(conn);
}


void Proxy_init(char *server, int port)
{
    SERVER = strdup(server);
    PORT = port;
}


void Proxy_connect(ProxyConnect *conn)
{
    taskcreate(proxytask, (void*)conn, STACK);
}

void
proxytask(void *v)
{
    ProxyConnect *h2l_conn = (ProxyConnect *)v;

    if((h2l_conn->read_fd = netdial(TCP, SERVER, PORT)) < 0){
        ProxyConnect_destroy(h2l_conn);
    }

    fdnoblock(h2l_conn->read_fd);
    fdnoblock(h2l_conn->write_fd);

    debug("Proxy connected to %s:%d", SERVER, PORT);
    
    ProxyConnect *l2h_conn = ProxyConnect_create(h2l_conn->read_fd, 
            malloc(h2l_conn->size), h2l_conn->size, 0);

    l2h_conn->read_fd = h2l_conn->write_fd;

    assert(l2h_conn->write_fd != h2l_conn->write_fd && "Wrong write fd setup.");
    assert(l2h_conn->read_fd != h2l_conn->read_fd && "Wrong read fd setup.");

    taskcreate(rwtask, (void *)l2h_conn, STACK);
    taskcreate(rwtask, (void *)h2l_conn, STACK);
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

