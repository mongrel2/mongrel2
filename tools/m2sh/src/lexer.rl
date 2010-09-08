#include "token.h"
#include "parser.h"

#include <stdio.h>

#include <bstring.h>
#include <dbg.h>
#include <adt/list.h>
#include <stdlib.h>


#define TK(N) debug("> " # N ": %.*s", (int)(te - ts), ts);\
    temp = calloc(sizeof(Token), 1);\
    temp->type = TK##N;\
    temp->data = blk2bstr(ts, (int)(te - ts));\
    list_append(token_list, lnode_create(temp)); 
     

%%{
    machine m2sh_lexer;

    comment = '#' [^\n]* '\n';

    action mark { m = fpc; }

    dqstring = '\"' (([^\\\"] | ('\\' any))*) '\"';
    sqstring = '\'' (([^\\\'] | ('\\' any))*) '\'';
    qstring = dqstring | sqstring;

    pattern = '`' (([^\\`] | ('\\' any))*) '`';
    ident = (alpha | '_')+ (alpha | digit | '_')*;
    class = [A-Z] alpha+;

    main := |*
        qstring { TK(QSTRING) };
        pattern { TK(PATTERN) };
        '=' { TK(EQ) };
        '{' { TK(LBRACKET) };
        '}' { TK(RBRACKET) };
        '[' { TK(LBRACE) };
        ']' { TK(RBRACE) };
        '(' { TK(LPAREN) };
        ')' { TK(RPAREN) };
        ',' { TK(COMMA) };
        ':' { TK(COLON) };

        space;
        comment;

        digit+ { TK(NUMBER) };
        class { TK(CLASS) };
        ident { TK(IDENT) };
    *|;
}%%

%% write data;

list_t *Lexer_tokenize(bstring content) 
{
    list_t *token_list = list_create(LISTCOUNT_T_MAX);
    check_mem(token_list);
    Token *temp;

    char *p = bdata(content);
    char *pe = p + blength(content);
    char *eof = pe;
    int cs = -1;
    int act = -1;
    char *ts = NULL;
    char *te = NULL;

    %% write init;
    %% write exec;

    if ( cs == %%{ write error; }%% ) {
        debug("ERROR AT: %d\n%s", (int)(pe - p), p);
    } else if ( cs >= %%{ write first_final; }%% ) {
        debug("FINISHED");
    } else {
        debug("NOT FINISHED");
    }

    return token_list;

error:
    list_destroy(token_list);
    return NULL;
}
