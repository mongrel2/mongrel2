#ifndef _KEGOGI_PARSER_H
#define _KEGOGI_PARSER_H

#include "kegogi.h"

#include <bstring.h>

#include "param.h"

typedef struct Send {
    bstring method;
    bstring host;
    bstring port;
    bstring uri;
    ParamDict *params;
} Send;

typedef struct Expect {
    bstring status_code;
    ParamDict *params;
} Expect;

typedef struct Command {
    Send send;
    Expect expect;
} Command;

int parse_kegogi_file(const char *path, Command commands[], int max_commands);



#endif
