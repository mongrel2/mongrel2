#include "config.h"
#include "parser.h"

#include <stdio.h>

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
  hash_t **out_settings
);


#define TK(N) debug("> " # N ": %.*s", (int)(te - ts), ts);\
    temp = calloc(sizeof(Token), 1);\
    temp->type = TK##N;\
    temp->data = blk2bstr(ts, (int)(te - ts));\
    Parse(parser, TK##N, temp, &settings);

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

hash_t *Parse_config_string(bstring content) 
{
    Token *temp = NULL;
    void *parser = ParseAlloc(malloc);
    check_mem(parser);
    hash_t *settings = NULL;

    char *p = bdata(content);
    char *pe = p + blength(content);
    char *eof = pe;
    int cs = -1;
    int act = -1;
    char *ts = NULL;
    char *te = NULL;

    %% write init;
    %% write exec;

    Parse(parser, TKEOF, NULL, &settings);
    Parse(parser, 0, 0, &settings);

    if ( cs == %%{ write error; }%% ) {
        debug("ERROR AT: %d\n%s", (int)(pe - p), p);
    } else if ( cs >= %%{ write first_final; }%% ) {
        debug("FINISHED");
    } else {
        debug("NOT FINISHED");
    }

    ParseFree(parser, free);
    return settings;

error:
    ParseFree(parser, free);
    return NULL;
}


int Parse_config_file(const char *path)
{
    FILE *script;
    bstring buffer = NULL;
    lnode_t *n = NULL;
    hash_t *settings = NULL;

    script = fopen(path, "r");
    check(script, "Failed to open file: %s", path);

    buffer = bread((bNread)fread, script);
    check_mem(buffer);

    fclose(script); script = NULL;

    settings = Parse_config_string(buffer);
    check(settings != NULL, "Failed to parse file: %s", path);

    bdestroy(buffer);
    buffer = NULL;

    debug("FINAL COUNT: %d", (int)hash_count(settings));
    return 0;

error:
    bdestroy(buffer);
    if(script) fclose(script);
    return -1;
}

