#ifndef _response_h
#define _response_h

#include <bstring.h>

extern struct tagbstring HTTP_404;
extern struct tagbstring HTTP_502;

extern struct tagbstring FLASH_RESPONSE;

int Response_send_error(int fd, bstring error);
int Response_send_socket_policy(int fd);

#endif
