#ifndef _listener_h
#define _listener_h

#include <server.h>
#include <request.h>
#include <state.h>
#include <proxy.h>

enum
{
	CONNECTION_STACK = 32 * 1024,
    BUFFER_SIZE = 2 * 1024
};


typedef struct Connection {
    Server *server;
    int fd;
    Request *req;
    int nread;
    size_t nparsed;
    int finished;
    int registered;
    int rport;
    ProxyConnect *proxy;

    State state;
    char remote[IPADDR_SIZE+1];
    char buf[BUFFER_SIZE+1];
} Connection;

void Connection_destroy(Connection *conn);

Connection *Connection_create(Server *srv, int fd, int rport, const char *remote);

void Connection_accept(Connection *conn);

void Connection_task(void *v);


int Connection_deliver(int to_fd, bstring buf);

int Connection_read_header(Connection *conn, Request *req);

#endif
