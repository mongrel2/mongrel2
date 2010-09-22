#ifndef _m2sh_cli_h
#define _m2sh_cli_h

#include <bstring.h>
#include "ast.h"
#include "commands.h"

enum CLITokens {
    TKBLOB = TKCOLON + 100,
    TKOPTION
};

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
