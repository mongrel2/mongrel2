#ifndef _listener_h
#define _listener_h
#include <stdlib.h>
#include <handler.h>
#include <proxy.h>

enum
{
	LISTENER_STACK = 32 * 1024
};


typedef struct Listener {
    Handler *handler;
    Proxy *proxy;
    char *remote;
    int fd;
} Listener;

void Listener_init();

void Listener_destroy(Listener *listener);

Listener *Listener_create(Handler *handler, Proxy *proxy, int fd, const char *remote);

void Listener_accept(Handler *base, Proxy *proxy, int fd, const char *remote);

int Listener_deliver(int to_fd, char *buffer, size_t len);

void Listener_task(void*);

#endif
