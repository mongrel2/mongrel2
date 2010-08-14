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
    ParamDict *defaults;
} CommandList;

int parse_kegogi_file(const char *path, CommandList *commandList);
TokenList *get_kegogi_tokens(bstring content);

#endif
