#ifndef _handler_h
#define _handler_h

#include <stdlib.h>
#include <bstring.h>

enum
{
    HANDLER_STACK = 100 * 1024
};

typedef struct Handler {
    void *send_socket;
    void *recv_socket;
    bstring send_ident;
    bstring recv_ident;
    bstring recv_spec;
    bstring send_spec;
} Handler;

void Handler_task(void *v);

int Handler_deliver(void *handler_socket, char *buffer, size_t len);

Handler *Handler_create(const char *send_spec, const char *send_ident,
        const char *recv_spec, const char *recv_ident);

void Handler_destroy(Handler *handler);

void *Handler_recv_create(const char *recv_spec, const char *uuid);

void *Handler_send_create(const char *send_spec, const char *identity);

void Handler_notify_leave(Handler *handler, int fd);

#endif
