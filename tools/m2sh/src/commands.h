#ifndef _m2sh_commands_h
#define _m2sh_commands_h

#include <bstring.h>
#include <adt/hash.h>
#include <adt/list.h>
#include "token.h"

int Command_run(bstring arguments);

#define MAX_TOKENS 100

typedef struct Command {
    bstring progname;
    bstring name;
    hash_t *options;
    list_t *extra;
    int error;
    Token *tokens[MAX_TOKENS];
    int token_count;
} Command;


#endif
