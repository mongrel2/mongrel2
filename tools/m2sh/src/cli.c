
#line 1 "src/cli.rl"
/**
 *
 * Copyright (c) 2010, Zed A. Shaw and Mongrel2 Project Contributors.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 * 
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 * 
 *     * Neither the name of the Mongrel2 Project, Zed A. Shaw, nor the names
 *       of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written
 *       permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


/*
 * Parse command line arguments.
 */

#include <stdio.h>
#include <string.h>
#include "cli.h"
#include <dbg.h>
#include "ast.h"
#include <stdlib.h>

#define TKBASE(N, S, E)\
    if ( fsm->token_count < MAX_TOKENS ) {\
        temp = Token_create(TK##N, S, (int)(E - S));\
        fsm->tokens[fsm->token_count] = temp;\
    }\
    fsm->token_count++;

#define TK(N) TKBASE(N, fsm->ts, fsm->te)
#define TKSTR(N) TKBASE(N, fsm->ts+1, fsm->te-3)
#define TKOPT(C) TKBASE(OPTION, fsm->ts+(C), fsm->te-(C*2))


#line 80 "src/cli.rl"



#line 66 "src/cli.c"
static const int params_start = 4;
static const int params_first_final = 4;
static const int params_error = -1;

static const int params_en_main = 4;


#line 83 "src/cli.rl"

void cli_params_init( struct params *fsm )
{
    fsm->act = -1;
    fsm->token_count = 0;
    fsm->curtk = 0;
	
#line 82 "src/cli.c"
	{
	 fsm->cs = params_start;
	 fsm->ts = 0;
	 fsm->te = 0;
	 fsm->act = 0;
	}

#line 90 "src/cli.rl"
}

void cli_params_execute( struct params *fsm, bstring data)
{
	const char *p = bdata(data);
	const char *pe = bdataofs(data, blength(data)); // include \0
    const char *eof = pe;
    Token *temp = NULL;

	
#line 101 "src/cli.c"
	{
	if ( p == pe )
		goto _test_eof;
	switch (  fsm->cs )
	{
tr0:
#line 77 "src/cli.rl"
	{{p = (( fsm->te))-1;}{ TK(BLOB); }}
	goto st4;
tr2:
#line 71 "src/cli.rl"
	{ fsm->te = p+1;{ TKSTR(QSTRING); }}
	goto st4;
tr7:
#line 74 "src/cli.rl"
	{ fsm->te = p+1;}
	goto st4;
tr13:
#line 1 "NONE"
	{	switch(  fsm->act ) {
	case 3:
	{{p = (( fsm->te))-1;} TKSTR(QSTRING); }
	break;
	case 7:
	{{p = (( fsm->te))-1;} TK(BLOB); }
	break;
	}
	}
	goto st4;
tr14:
#line 77 "src/cli.rl"
	{ fsm->te = p;p--;{ TK(BLOB); }}
	goto st4;
tr21:
#line 70 "src/cli.rl"
	{ fsm->te = p;p--;{  TKOPT(2); }}
	goto st4;
tr22:
#line 69 "src/cli.rl"
	{ fsm->te = p;p--;{  TKOPT(1); }}
	goto st4;
tr23:
#line 73 "src/cli.rl"
	{ fsm->te = p;p--;{ TK(IDENT); }}
	goto st4;
tr24:
#line 72 "src/cli.rl"
	{ fsm->te = p;p--;{ TK(NUMBER); }}
	goto st4;
st4:
#line 1 "NONE"
	{ fsm->ts = 0;}
	if ( ++p == pe )
		goto _test_eof4;
case 4:
#line 1 "NONE"
	{ fsm->ts = p;}
#line 159 "src/cli.c"
	switch( (*p) ) {
		case 32: goto tr7;
		case 34: goto tr8;
		case 39: goto tr9;
		case 45: goto st10;
		case 46: goto st14;
		case 95: goto st14;
	}
	if ( (*p) < 48 ) {
		if ( 9 <= (*p) && (*p) <= 13 )
			goto tr7;
	} else if ( (*p) > 57 ) {
		if ( (*p) > 90 ) {
			if ( 97 <= (*p) && (*p) <= 122 )
				goto st14;
		} else if ( (*p) >= 65 )
			goto st14;
	} else
		goto st15;
	goto tr6;
tr6:
#line 1 "NONE"
	{ fsm->te = p+1;}
#line 77 "src/cli.rl"
	{ fsm->act = 7;}
	goto st5;
tr15:
#line 1 "NONE"
	{ fsm->te = p+1;}
#line 71 "src/cli.rl"
	{ fsm->act = 3;}
	goto st5;
st5:
	if ( ++p == pe )
		goto _test_eof5;
case 5:
#line 196 "src/cli.c"
	if ( (*p) == 32 )
		goto tr13;
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr13;
	goto tr6;
tr8:
#line 1 "NONE"
	{ fsm->te = p+1;}
	goto st6;
st6:
	if ( ++p == pe )
		goto _test_eof6;
case 6:
#line 210 "src/cli.c"
	switch( (*p) ) {
		case 32: goto st0;
		case 34: goto tr15;
		case 92: goto tr16;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto st0;
	goto tr8;
st0:
	if ( ++p == pe )
		goto _test_eof0;
case 0:
	switch( (*p) ) {
		case 34: goto tr2;
		case 92: goto st1;
	}
	goto st0;
st1:
	if ( ++p == pe )
		goto _test_eof1;
case 1:
	goto st0;
tr16:
#line 1 "NONE"
	{ fsm->te = p+1;}
	goto st7;
st7:
	if ( ++p == pe )
		goto _test_eof7;
case 7:
#line 241 "src/cli.c"
	if ( (*p) == 32 )
		goto st0;
	if ( 9 <= (*p) && (*p) <= 13 )
		goto st0;
	goto tr8;
tr9:
#line 1 "NONE"
	{ fsm->te = p+1;}
	goto st8;
st8:
	if ( ++p == pe )
		goto _test_eof8;
case 8:
#line 255 "src/cli.c"
	switch( (*p) ) {
		case 32: goto st2;
		case 39: goto tr15;
		case 92: goto tr17;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto st2;
	goto tr9;
st2:
	if ( ++p == pe )
		goto _test_eof2;
case 2:
	switch( (*p) ) {
		case 39: goto tr2;
		case 92: goto st3;
	}
	goto st2;
st3:
	if ( ++p == pe )
		goto _test_eof3;
case 3:
	goto st2;
tr17:
#line 1 "NONE"
	{ fsm->te = p+1;}
	goto st9;
st9:
	if ( ++p == pe )
		goto _test_eof9;
case 9:
#line 286 "src/cli.c"
	if ( (*p) == 32 )
		goto st2;
	if ( 9 <= (*p) && (*p) <= 13 )
		goto st2;
	goto tr9;
st10:
	if ( ++p == pe )
		goto _test_eof10;
case 10:
	switch( (*p) ) {
		case 32: goto tr14;
		case 45: goto st11;
		case 46: goto st13;
		case 95: goto st13;
	}
	if ( (*p) < 65 ) {
		if ( 9 <= (*p) && (*p) <= 13 )
			goto tr14;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st13;
	} else
		goto st13;
	goto tr6;
st11:
	if ( ++p == pe )
		goto _test_eof11;
case 11:
	switch( (*p) ) {
		case 32: goto tr14;
		case 46: goto st12;
		case 95: goto st12;
	}
	if ( (*p) < 65 ) {
		if ( 9 <= (*p) && (*p) <= 13 )
			goto tr14;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st12;
	} else
		goto st12;
	goto tr6;
st12:
	if ( ++p == pe )
		goto _test_eof12;
case 12:
	switch( (*p) ) {
		case 32: goto tr21;
		case 46: goto st12;
		case 95: goto st12;
	}
	if ( (*p) < 48 ) {
		if ( 9 <= (*p) && (*p) <= 13 )
			goto tr21;
	} else if ( (*p) > 57 ) {
		if ( (*p) > 90 ) {
			if ( 97 <= (*p) && (*p) <= 122 )
				goto st12;
		} else if ( (*p) >= 65 )
			goto st12;
	} else
		goto st12;
	goto tr6;
st13:
	if ( ++p == pe )
		goto _test_eof13;
case 13:
	switch( (*p) ) {
		case 32: goto tr22;
		case 46: goto st13;
		case 95: goto st13;
	}
	if ( (*p) < 48 ) {
		if ( 9 <= (*p) && (*p) <= 13 )
			goto tr22;
	} else if ( (*p) > 57 ) {
		if ( (*p) > 90 ) {
			if ( 97 <= (*p) && (*p) <= 122 )
				goto st13;
		} else if ( (*p) >= 65 )
			goto st13;
	} else
		goto st13;
	goto tr6;
st14:
	if ( ++p == pe )
		goto _test_eof14;
case 14:
	switch( (*p) ) {
		case 32: goto tr23;
		case 46: goto st14;
		case 95: goto st14;
	}
	if ( (*p) < 48 ) {
		if ( 9 <= (*p) && (*p) <= 13 )
			goto tr23;
	} else if ( (*p) > 57 ) {
		if ( (*p) > 90 ) {
			if ( 97 <= (*p) && (*p) <= 122 )
				goto st14;
		} else if ( (*p) >= 65 )
			goto st14;
	} else
		goto st14;
	goto tr6;
st15:
	if ( ++p == pe )
		goto _test_eof15;
case 15:
	if ( (*p) == 32 )
		goto tr24;
	if ( (*p) > 13 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st15;
	} else if ( (*p) >= 9 )
		goto tr24;
	goto tr6;
	}
	_test_eof4:  fsm->cs = 4; goto _test_eof; 
	_test_eof5:  fsm->cs = 5; goto _test_eof; 
	_test_eof6:  fsm->cs = 6; goto _test_eof; 
	_test_eof0:  fsm->cs = 0; goto _test_eof; 
	_test_eof1:  fsm->cs = 1; goto _test_eof; 
	_test_eof7:  fsm->cs = 7; goto _test_eof; 
	_test_eof8:  fsm->cs = 8; goto _test_eof; 
	_test_eof2:  fsm->cs = 2; goto _test_eof; 
	_test_eof3:  fsm->cs = 3; goto _test_eof; 
	_test_eof9:  fsm->cs = 9; goto _test_eof; 
	_test_eof10:  fsm->cs = 10; goto _test_eof; 
	_test_eof11:  fsm->cs = 11; goto _test_eof; 
	_test_eof12:  fsm->cs = 12; goto _test_eof; 
	_test_eof13:  fsm->cs = 13; goto _test_eof; 
	_test_eof14:  fsm->cs = 14; goto _test_eof; 
	_test_eof15:  fsm->cs = 15; goto _test_eof; 

	_test_eof: {}
	if ( p == eof )
	{
	switch (  fsm->cs ) {
	case 5: goto tr13;
	case 6: goto tr14;
	case 0: goto tr0;
	case 1: goto tr0;
	case 7: goto tr14;
	case 8: goto tr14;
	case 2: goto tr0;
	case 3: goto tr0;
	case 9: goto tr14;
	case 10: goto tr14;
	case 11: goto tr14;
	case 12: goto tr21;
	case 13: goto tr22;
	case 14: goto tr23;
	case 15: goto tr24;
	}
	}

	}

#line 100 "src/cli.rl"
}

int cli_params_finish( struct params *fsm )
{
	if ( fsm->token_count > MAX_TOKENS )
		return -2;
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
    memcpy(cmd->tokens, p->tokens, sizeof(cmd->tokens));
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


