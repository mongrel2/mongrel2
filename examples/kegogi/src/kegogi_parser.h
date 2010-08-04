#ifndef _KEGOGI_PARSER_H
#define _KEGOGI_PARSER_H

#include "kegogi.h"

#include <bstring.h>

typedef struct Send {
    bstring method;
    bstring host;
    bstring port;
    bstring uri;
} Send;

typedef struct Expect {
    bstring status_code;
} Expect;

typedef struct Command {
    Send send;
    Expect expect;
} Command;

int parse_kegogi_file(const char *path, Command commands[], int max_commands);



#endif
