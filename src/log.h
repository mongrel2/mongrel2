#ifndef _log_h
#define _log_h

#include "server.h"
#include "connection.h"

int Log_init(Server *server);

int Log_request(Connection *conn, int status, int size);

#endif
