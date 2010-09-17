
/*
 * Parse command line arguments.
 */

#include <stdio.h>
#include <string.h>
#include "cli.h"
#include <dbg.h>
#include "ast.h"
#include <stdlib.h>

#define TKBASE(N, S, E) temp = calloc(sizeof(Token), 1);\
    temp->type = TK##N;\
    temp->data = blk2bstr(S, (int)(E - S));\
    fsm->tokens[fsm->token_count++] = temp;

#define TK(N) TKBASE(N, fsm->ts, fsm->te)
#define TKSTR(N) TKBASE(N, fsm->ts+1, fsm->te-3)
#define TKOPT(C) TKBASE(OPTION, fsm->ts+(C), fsm->te-(C*2))

%%{
	machine params;
	access fsm->;

    dqstring = '\"' (([^\\\"] | ('\\' any))*) '\"';
    sqstring = '\'' (([^\\\'] | ('\\' any))*) '\'';
    pattern = '`' (([^\\`] | ('\\' any))*) '`';
    ident = (alpha | '_' | '.')+ (alpha | digit | '_' | '.')*;
    blob = (any -- space)*;

    main := |*
        '-' ident {  TKOPT(1); };
        '--' ident {  TKOPT(2); };
        dqstring | sqstring { TKSTR(QSTRING); };
        digit+ { TK(NUMBER); };
        ident { TK(IDENT); };
        space;

        # anything else we can't identify just slurp it up until the next space
        blob { TK(BLOB); };
    *|;

}%%

%% write data;

void cli_params_init( struct params *fsm )
{
    fsm->act = -1;
    fsm->token_count = 0;
    fsm->curtk = 0;
	%% write init;
}

void cli_params_execute( struct params *fsm, bstring data)
{
	const char *p = bdata(data);
	const char *pe = bdataofs(data, blength(data)); // include \0
    const char *eof = pe;
    Token *temp = NULL;

	%% write exec;
}

int cli_params_finish( struct params *fsm )
{
	if ( fsm->cs == params_error )
		return -1;
	if ( fsm->cs >= params_first_final )
		return 1;
	return 0;
}

static inline bstring match(struct params *p, int type)
{
    check(p->curtk < p->token_count, "Expecting %d but nothing left.", type);
    Token *tk = p->tokens[p->curtk++];

    check(tk->type == type, "Expecting %d but got %d.", type, tk->type);

    return tk->data;

error:
    return NULL;
}


static inline int peek(struct params *p)
{
    if(p->curtk >= p->token_count) {
        return -1;
    } else {
        return p->tokens[p->curtk]->type;
    }
}

static inline void Command_option(Command *cmd, struct params *p)
{
    bstring key = match(p, TKOPTION);
    check(key, "Should have matched an option.");

    bstring value = NULL;
    int next = peek(p);

    if(next == TKOPTION || next == -1) {
        // simple true/false setting
        value = bfromcstr("");
    } else {
        // it's not an option so it's some value we want
        value = match(p, next);
    }

    hash_alloc_insert(cmd->options, bdata(key), value);
    return;

error:
    cmd->error = 1;
    return;
}

static inline void Command_extra(Command *cmd, int next, struct params *p)
{
    bstring extra = match(p, next);
    list_append(cmd->extra, lnode_create(extra));
}


static inline int Command_parse(struct params *p, Command *cmd)
{
    int next = 0;

    cmd->options = hash_create(HASHCOUNT_T_MAX, 0, 0);
    check_mem(cmd->options);

    cmd->extra = list_create(LISTCOUNT_T_MAX);
    check_mem(cmd->extra);

    next = peek(p);
    if(next == TKIDENT || next == TKBLOB) {
        cmd->progname = match(p, next);
        check(cmd->progname, "No program name given in command.");
    } else {
        sentinel("Expected the name of the program you're running.");
    }

    cmd->name = match(p, TKIDENT);
    check(cmd->name, "No command name given.");

    for(next = peek(p); next != -1 && !cmd->error; next = peek(p)) {
        if(next == TKOPTION) {
            Command_option(cmd, p);
        } else {
            Command_extra(cmd, next, p);
        }
    }

    check(p->curtk == p->token_count, "Didn't parse it all, only: %d of %d", p->curtk, p->token_count);

    return 0;

error:
    cmd->error = 1;
    return -1;
}

int cli_params_parse_args(bstring args, Command *cmd)
{
	int i = 0;
	struct params params;
    cmd->error = 0;

	cli_params_init(&params);

    cli_params_execute(&params, args);

    int rc = cli_params_finish(&params);
    check(rc == 1, "error processing arguments: %d", rc);

    check(params.token_count >= 2, "No command given.");

    return Command_parse(&params, cmd);

error:
    cmd->error = 1;
    return -1;
}
