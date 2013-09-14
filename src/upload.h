#ifndef _upload_h
#define _upload_h

#include "connection.h"
#include "handler.h"

int Upload_file(Connection *conn, Handler *handler, int content_len);
int Upload_notify(Connection *conn, Handler *handler, const char *stage, bstring tmp_name);

int Upload_stream(Connection *conn, Handler *handler, int content_len);

#endif
