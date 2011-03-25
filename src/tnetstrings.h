#ifndef _tnetstrings_h
#define _tnetstrings_h

#include "bstring.h"

int tnetstring_decode(bstring payload, const char *spec, ...);

int tnetstring_encode(bstring payload, const char *spec, ...);

#define tns_start() bstring tns_stack = bfromcstr(""); bstring tns_temp = NULL;
#define tns_push(S, ...) tnetstring_encode(tns_stack, S, ##__VA_ARGS__)
#define tns_pop(C)\
        tns_temp = bformat("%d:%s%c", blength(tns_stack), bdata(tns_stack), C);\
        bdestroy(tns_stack);\
        tns_stack = tns_temp;
        
#define tns_top() tns_stack
#define tns_done() bdestroy(tns_stack)
#define tns_clear() btrunc(tns_stack, 0);

#endif

