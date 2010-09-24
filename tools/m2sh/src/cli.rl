
/*
 * Parse command line arguments.
 */

#include <stdio.h>
#include <string.h>
#include "cli.h"
#include <dbg.h>
#include "ast.h"
#include <stdlib.h>

#define TKBASE(N, S, E) temp = Token_create(TK##N, S, (int)(E - S));\
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

static inline bstring match_release(struct params *p, int type, int release)
{
    check(p->curtk < p->token_count, "Expecting more options, but nothing left.");
    Token *tk = p->tokens[p->curtk++];

    check_debug(tk->type == type, "Expecting %d but got %d.", type, tk->type);

    bstring val = tk->data;

    if(release) {
        tk->data = NULL;  // that when it's freed we don't try again
    }

    return val;

error:
    return NULL;
}

static inline bstring match(struct params *p, int type)
{
    return match_release(p, type, 1);
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
    bstring key = match_release(p, TKOPTION, 0);
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
    bstring extra = match_release(p, next, 0);
    list_append(cmd->extra, lnode_create(extra));
}

hnode_t *cmd_hnode_alloc(void *ignored)
{
    return calloc(sizeof(hnode_t), 1);
}

void cmd_hnode_free(hnode_t *h, void *ignored)
{
    bdestroy(hnode_get(h));
    hnode_destroy(h);
}

static inline int Command_parse(struct params *p, Command *cmd)
{
    int next = 0;

    // TODO: refactor this, but for now command takes over the tokens until it's done
    memcpy(cmd->tokens, p->tokens, MAX_TOKENS);
    cmd->token_count = p->token_count;

    cmd->options = hash_create(HASHCOUNT_T_MAX, 0, 0);
    check_mem(cmd->options);
    hash_set_allocator(cmd->options, cmd_hnode_alloc, cmd_hnode_free, NULL);

    cmd->extra = list_create(LISTCOUNT_T_MAX);
    check_mem(cmd->extra);

    next = peek(p);
    if(next == TKIDENT || next == TKBLOB) {
        cmd->progname = match(p, next);
        check(cmd->progname, "No program name given in command.");
    } else {
        sentinel("Expected the name of the program you're running not: %s",
                bdata(match(p, next)));
    }

    cmd->name = match(p, TKIDENT);
    check(cmd->name, "No command name given.  Use m2sh help to figure out what's available.");

    for(next = peek(p); next != -1 && !cmd->error; next = peek(p)) {
        if(next == TKOPTION) {
            Command_option(cmd, p);
        } else {
            Command_extra(cmd, next, p);
        }
    }

    check(p->curtk == p->token_count, "Didn't parse the whole command line, only: %d of %d", p->curtk, p->token_count);

    return 0;

error:
    cmd->error = 1;
    return -1;
}

struct tagbstring DEFAULT_COMMAND = bsStatic("shell");

int cli_params_parse_args(bstring args, Command *cmd)
{
	struct params params;
    cmd->error = 0;
    cmd->token_count = 0;

	cli_params_init(&params);

    cli_params_execute(&params, args);

    int rc = cli_params_finish(&params);
    check(rc == 1, "error processing arguments: %d", rc);

    if(params.token_count < 2) {
        params.tokens[params.token_count++] =  Token_create(
                TKIDENT, bdata(&DEFAULT_COMMAND), blength(&DEFAULT_COMMAND));
    }

    return Command_parse(&params, cmd);

error:
    cmd->error = 1;
    return -1;
}


