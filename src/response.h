#ifndef _response_h
#define _response_h

#include <bstring.h>

extern struct tagbstring HTTP_304;
extern struct tagbstring HTTP_400;
extern struct tagbstring HTTP_404;
extern struct tagbstring HTTP_405;
extern struct tagbstring HTTP_412;
extern struct tagbstring HTTP_413;
extern struct tagbstring HTTP_500;
extern struct tagbstring HTTP_501;
extern struct tagbstring HTTP_502;

extern struct tagbstring FLASH_RESPONSE;

int Response_send_status(int fd, bstring error);

int Response_send_socket_policy(int fd);

#endif
