#ifndef __dbg_h__
#define __dbg_h__

#include <stdio.h>
#include <errno.h>

extern FILE *LOG_FILE;

#define debug(M, ...) fprintf(LOG_FILE, "DEBUG %s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)


#define log_err(M, ...) fprintf(LOG_FILE, "ERROR (%s:%d: errno: %s) " M "\n", __FILE__, __LINE__, strerror(errno), ##__VA_ARGS__)

#define check(A, M, ...) if(!(A)) { log_err(M, ##__VA_ARGS__); errno=0; goto error; }

#endif
