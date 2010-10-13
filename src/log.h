#ifndef _log_h
#define _log_h

#include "server.h"
#include "connection.h"

int Log_init(bstring access_log, bstring log_spec);

int Log_bind(const char *endpoint);

int Log_poison_workers();

int Log_request(Connection *conn, int status, int size);

int Log_term();

#endif
