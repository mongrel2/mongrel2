#ifndef cli_h
#define cli_h

#include <bstring.h>
#include "ast.h"

#define MAX_TOKENS 100
enum CLITokens {
    TKBLOB = TKCOLON + 100,
    TKOPTION
};

typedef struct Command {
    bstring progname;
    bstring name;
    hash_t *options;
    list_t *extra;
    int error;
} Command;

struct params
{
	int cs;
    const char *ts;
    const char *te;
    int act;

    Token *tokens[MAX_TOKENS];
    int token_count;
    int curtk;
};

void cli_params_init( struct params *fsm );
void cli_params_execute( struct params *fsm, bstring data);
int cli_params_finish( struct params *fsm );
int cli_params_parse_args(bstring args, Command *cmd);

#endif
