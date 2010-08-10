#ifndef _KEGOGI_H
#define _KEGOGI_H

#include <bstring.h>

#include "param.h"
#include "kegogi_tokens.h"

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

typedef struct CommandList {
    int size;
    int count;
    Command *commands;
} CommandList;

TokenList *get_kegogi_tokens(bstring content);


#endif
