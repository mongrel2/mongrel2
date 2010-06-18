#ifndef _listener_h
#define _listener_h
#include <stdlib.h>

typedef struct SocketPair {
    void *listener;
    void *handler;
    int fd;
} SocketPair;

enum
{
	LISTENER_STACK = 32 * 1024
};

int Listener_deliver(int to_fd, char *buffer, size_t len);
void Listener_task(void*);
void *Listener_create(const char *listener_spec, const char *uuid);

#endif
