#ifndef _upload_h
#define _upload_h

#include "connection.h"
#include "handler.h"
#include "bstring.h"

bstring Upload_file(Connection *conn, Handler *handler, int content_len);

#endif
