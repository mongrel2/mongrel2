#ifndef _register_h
#define _register_h

enum {
    CONN_TYPE_HTTP=1,
    CONN_TYPE_MSG,
    CONN_TYPE_SOCKET
};

void Register_connect(int fd, int conn_type);

void Register_disconnect(int fd);

int Register_ping(int fd);

void Register_init();

int Register_exists(int fd);

#endif
