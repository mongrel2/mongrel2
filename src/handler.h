#ifndef _handler_h
#define _handler_h

#include <stdlib.h>

enum
{
    HANDLER_STACK = 100 * 1024
};


void Handler_task(void *v);
int Handler_deliver(void *handler_socket, int from_fd, char *buffer, size_t len);
void *Handler_create(const char *handler_spec, const char *identity);
void Handler_notify_leave(void *handler_socket, int fd);

#endif
