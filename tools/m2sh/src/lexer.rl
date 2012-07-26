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

%%{
    machine m2sh_lexer;

    comment = '#' [^\n]* '\n';

    action mark { m = fpc; }

    dqstring = '\"' (([^\\\"] | ('\\' any))*) '\"';
    sqstring = '\'' (([^\\\'] | ('\\' any))*) '\'';
    qstring = dqstring | sqstring;

    ident = (alpha | '_')+ (alpha | digit | '_')*;
    class = [A-Z] alpha+;

    main := |*
        qstring { TKSTR(QSTRING) };
        '=' { TK(EQ) };
        '{' { TK(LBRACKET) };
        '}' { TK(RBRACKET) };
        '[' { TK(LBRACE) };
        ']' { TK(RBRACE) };
        '(' { TK(LPAREN) };
        ')' { TK(RPAREN) };
        ',' { TK(COMMA) };
        ':' { TK(COLON) };

        '\n' { state.line_number++; };
        space -- '\n';

        comment;

        digit+ { TK(NUMBER) };
        class { TK(CLASS) };
        ident { TK(IDENT) };
    *|;
}%%

%% write data;

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

    %% write init;
    %% write exec;


    if(state.error) {
        Parse_print_error("SYNTAX ERROR", content, 
                (int)(ts - bdata(content)), ++state.line_number);
    } else if( cs == %%{ write error; }%% ) {
        Parse_print_error("INVALID CHARACTER", content,
                (int)(ts - bdata(content)), ++state.line_number);
    } else if( cs >= %%{ write first_final; }%% ) {
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

