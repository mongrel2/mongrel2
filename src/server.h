#ifndef _server_h
#define _server_h

#include <adt/tst.h>
#include <adt/hash.h>
#include <handler.h>

typedef struct Server {
    int listener_port;
    Handler *handler;
    Proxy *proxy;
} Server;


#endif
