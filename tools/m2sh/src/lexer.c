
#line 1 "src/lexer.rl"
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

#include "config_file.h"
#include "parser.h"

#include <stdio.h>
#include <assert.h>
#include <bstring.h>
#include <dbg.h>
#include <adt/list.h>
#include <stdlib.h>

void *ParseAlloc(void *(*mallocProc)(size_t));
void ParseFree(void *p, void (*freeProc)(void*));

void Parse(
  void *yyp,                   /* The parser */
  int yymajor,                 /* The major token code number */
  Token *yyminor,       /* The value for the token */
  ParserState *state
);

#define TKBASE(N, S, E) temp = Token_create(TK##N, S, (int)(E - S));\
    Parse(parser, TK##N, temp, &state);\
    if(state.error) goto error;

#define TK(N) TKBASE(N, ts, te)
#define TKSTR(N) TKBASE(N, ts+1, te-3)


#line 97 "src/lexer.rl"



#line 70 "src/lexer.c"
static const int m2sh_lexer_start = 6;
static const int m2sh_lexer_first_final = 6;
static const int m2sh_lexer_error = 0;

static const int m2sh_lexer_en_main = 6;


#line 100 "src/lexer.rl"

void Parse_print_error(const char *message, bstring content, int at, int line_number)
{
    int prev_nl = bstrrchrp(content, '\n', at);
    int next_nl = bstrchrp(content, '\n', at);

    if(prev_nl < 0) {
        log_err("%s AT '%c' on line %d:\n%.*s\n%*s", message,
                bchar(content, at), line_number-1, 
                next_nl, bdata(content),
                at, "^");
    } else {
        log_err("%s AT '%c' on line %d:%.*s\n%*s", message,
                bchar(content, at), line_number, 
                next_nl - prev_nl, bdataofs(content, prev_nl),
                at - prev_nl, "^");
    }
}

tst_t *Parse_config_string(bstring content) 
{
    Token *temp = NULL;
    void *parser = ParseAlloc(malloc);
    check_mem(parser);
    ParserState state = {.settings = NULL, .error = 0, .line_number = 1};

    char *p = bdata(content);
    char *pe = bdataofs(content, blength(content) - 1);
    char *eof = pe;
    int cs = -1;
    int act = -1;
    char *ts = NULL;
    char *te = NULL;

    
#line 114 "src/lexer.c"
	{
	cs = m2sh_lexer_start;
	ts = 0;
	te = 0;
	act = 0;
	}

#line 135 "src/lexer.rl"
    
#line 124 "src/lexer.c"
	{
	if ( p == pe )
		goto _test_eof;
	switch ( cs )
	{
tr1:
#line 77 "src/lexer.rl"
	{te = p+1;{ TKSTR(QSTRING) }}
	goto st6;
tr4:
#line 91 "src/lexer.rl"
	{te = p+1;}
	goto st6;
tr7:
#line 89 "src/lexer.rl"
	{te = p+1;}
	goto st6;
tr9:
#line 88 "src/lexer.rl"
	{te = p+1;{ state.line_number++; }}
	goto st6;
tr10:
#line 83 "src/lexer.rl"
	{te = p+1;{ TK(LPAREN) }}
	goto st6;
tr11:
#line 84 "src/lexer.rl"
	{te = p+1;{ TK(RPAREN) }}
	goto st6;
tr12:
#line 85 "src/lexer.rl"
	{te = p+1;{ TK(COMMA) }}
	goto st6;
tr14:
#line 86 "src/lexer.rl"
	{te = p+1;{ TK(COLON) }}
	goto st6;
tr15:
#line 78 "src/lexer.rl"
	{te = p+1;{ TK(EQ) }}
	goto st6;
tr17:
#line 81 "src/lexer.rl"
	{te = p+1;{ TK(LBRACE) }}
	goto st6;
tr18:
#line 82 "src/lexer.rl"
	{te = p+1;{ TK(RBRACE) }}
	goto st6;
tr20:
#line 79 "src/lexer.rl"
	{te = p+1;{ TK(LBRACKET) }}
	goto st6;
tr21:
#line 80 "src/lexer.rl"
	{te = p+1;{ TK(RBRACKET) }}
	goto st6;
tr22:
#line 93 "src/lexer.rl"
	{te = p;p--;{ TK(NUMBER) }}
	goto st6;
tr23:
#line 1 "NONE"
	{	switch( act ) {
	case 15:
	{{p = ((te))-1;} TK(CLASS) }
	break;
	case 16:
	{{p = ((te))-1;} TK(IDENT) }
	break;
	}
	}
	goto st6;
tr25:
#line 95 "src/lexer.rl"
	{te = p;p--;{ TK(IDENT) }}
	goto st6;
st6:
#line 1 "NONE"
	{ts = 0;}
	if ( ++p == pe )
		goto _test_eof6;
case 6:
#line 1 "NONE"
	{ts = p;}
#line 210 "src/lexer.c"
	switch( (*p) ) {
		case 10: goto tr9;
		case 32: goto tr7;
		case 34: goto st1;
		case 35: goto st3;
		case 39: goto st4;
		case 40: goto tr10;
		case 41: goto tr11;
		case 44: goto tr12;
		case 58: goto tr14;
		case 61: goto tr15;
		case 91: goto tr17;
		case 93: goto tr18;
		case 95: goto st9;
		case 123: goto tr20;
		case 125: goto tr21;
	}
	if ( (*p) < 48 ) {
		if ( 9 <= (*p) && (*p) <= 13 )
			goto tr7;
	} else if ( (*p) > 57 ) {
		if ( (*p) > 90 ) {
			if ( 97 <= (*p) && (*p) <= 122 )
				goto st9;
		} else if ( (*p) >= 65 )
			goto tr16;
	} else
		goto st7;
	goto st0;
st0:
cs = 0;
	goto _out;
st1:
	if ( ++p == pe )
		goto _test_eof1;
case 1:
	switch( (*p) ) {
		case 34: goto tr1;
		case 92: goto st2;
	}
	goto st1;
st2:
	if ( ++p == pe )
		goto _test_eof2;
case 2:
	goto st1;
st3:
	if ( ++p == pe )
		goto _test_eof3;
case 3:
	if ( (*p) == 10 )
		goto tr4;
	goto st3;
st4:
	if ( ++p == pe )
		goto _test_eof4;
case 4:
	switch( (*p) ) {
		case 39: goto tr1;
		case 92: goto st5;
	}
	goto st4;
st5:
	if ( ++p == pe )
		goto _test_eof5;
case 5:
	goto st4;
st7:
	if ( ++p == pe )
		goto _test_eof7;
case 7:
	if ( 48 <= (*p) && (*p) <= 57 )
		goto st7;
	goto tr22;
tr16:
#line 1 "NONE"
	{te = p+1;}
#line 95 "src/lexer.rl"
	{act = 16;}
	goto st8;
tr24:
#line 1 "NONE"
	{te = p+1;}
#line 94 "src/lexer.rl"
	{act = 15;}
	goto st8;
st8:
	if ( ++p == pe )
		goto _test_eof8;
case 8:
#line 301 "src/lexer.c"
	if ( (*p) == 95 )
		goto st9;
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st9;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto tr24;
	} else
		goto tr24;
	goto tr23;
st9:
	if ( ++p == pe )
		goto _test_eof9;
case 9:
	if ( (*p) == 95 )
		goto st9;
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st9;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st9;
	} else
		goto st9;
	goto tr25;
	}
	_test_eof6: cs = 6; goto _test_eof; 
	_test_eof1: cs = 1; goto _test_eof; 
	_test_eof2: cs = 2; goto _test_eof; 
	_test_eof3: cs = 3; goto _test_eof; 
	_test_eof4: cs = 4; goto _test_eof; 
	_test_eof5: cs = 5; goto _test_eof; 
	_test_eof7: cs = 7; goto _test_eof; 
	_test_eof8: cs = 8; goto _test_eof; 
	_test_eof9: cs = 9; goto _test_eof; 

	_test_eof: {}
	if ( p == eof )
	{
	switch ( cs ) {
	case 7: goto tr22;
	case 8: goto tr23;
	case 9: goto tr25;
	}
	}

	_out: {}
	}

#line 136 "src/lexer.rl"


    if(state.error) {
        Parse_print_error("SYNTAX ERROR", content, 
                (int)(ts - bdata(content)), ++state.line_number);
    } else if( cs == 
#line 359 "src/lexer.c"
0
#line 141 "src/lexer.rl"
 ) {
        Parse_print_error("INVALID CHARACTER", content,
                (int)(ts - bdata(content)), ++state.line_number);
    } else if( cs >= 
#line 366 "src/lexer.c"
6
#line 144 "src/lexer.rl"
 ) {
        Parse(parser, TKEOF, NULL, &state);
    } else {
        log_err("INCOMPLETE CONFIG FILE. There needs to be more to this.");
    }

    Parse(parser, 0, 0, &state);
    ParseFree(parser, free);

    return state.settings;

error:
    if(state.error) {
        Parse_print_error("SYNTAX ERROR", content, 
                (int)(ts - bdata(content)), ++state.line_number);
    }
    ParseFree(parser, free);
    return NULL;
}


tst_t *Parse_config_file(const char *path)
{
    FILE *script;
    bstring buffer = NULL;
    tst_t *settings = NULL;

    script = fopen(path, "r");
    check(script, "Failed to open file: %s", path);

    buffer = bread((bNread)fread, script);
    check_mem(buffer);

    fclose(script); script = NULL;

    // make sure there's a \n at the end
    bconchar(buffer, '\n');

    settings = Parse_config_string(buffer);
    check(settings != NULL, "Failed to parse file: %s", path);

    bdestroy(buffer);
    buffer = NULL;

    return settings;

error:
    bdestroy(buffer);
    if(script) fclose(script);
    return NULL;
}

