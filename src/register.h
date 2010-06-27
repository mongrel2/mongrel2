#ifndef _register_h
#define _register_h

void Register_connect(int fd);

void Register_disconnect(int fd);

int Register_ping(int fd);

void Register_init();

int Register_exists(int fd);

#endif
