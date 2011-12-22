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

bstring option(Command *cmd, const char *name, const char *def);
int log_action(bstring db_file, bstring what, bstring why, bstring where, bstring how);

#define check_file(S, N, P) check(access((const char *)(S)->data, (P)) == 0, "Can't access %s '%s' properly.", N, bdata((S)))

#endif
