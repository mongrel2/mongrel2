#ifndef __dbg_h__
#define __dbg_h__

#include <stdio.h>
#include <errno.h>
#include <string.h>

extern FILE *LOG_FILE;

#ifdef NDEBUG
#define debug(M, ...)
#else
#define debug(M, ...) fprintf(LOG_FILE, "DEBUG %s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#endif


// do not try to be smart and make this go away on NDEBUG, the _debug
// here means that it just doesn't print a message, it still does the
// check.  MKAY?
#define check_debug(A, M, ...) if(!(A)) { debug(M, ##__VA_ARGS__); errno=0; goto error; }

#define log_err(M, ...) fprintf(LOG_FILE, "ERROR (%s:%d: errno: %s) " M "\n", __FILE__, __LINE__, strerror(errno), ##__VA_ARGS__)

#define check(A, M, ...) if(!(A)) { log_err(M, ##__VA_ARGS__); errno=0; goto error; }

#define sentinel(M, ...)  { log_err(M, ##__VA_ARGS__); errno=0; goto error; }

#define log_info(M, ...) fprintf(LOG_FILE, "INFO (%s:%d) " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)

#define check_mem(A) check((A), "Out of memory.")

#endif
