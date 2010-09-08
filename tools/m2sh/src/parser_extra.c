#include <stdio.h>
#include "token.h"
#include "parser.h"
#include <dbg.h>
#include <stdlib.h>
#include <adt/list.h>

int parse_file(const char *path)
{
    FILE *script;
    list_t *tokens = NULL;
    bstring buffer = NULL;
    void *parser = NULL;
    lnode_t *n = NULL;
    hash_t *settings = NULL;

    script = fopen(path, "r");
    check(script, "Failed to open file: %s", path);

    buffer = bread((bNread)fread, script);
    check_mem(buffer);

    fclose(script);
    script = NULL;

    tokens = Lexer_tokenize(buffer);
    check_mem(tokens);

    bdestroy(buffer);
    buffer = NULL;

    parser = ParseAlloc(malloc);
    check_mem(parser);

    for(n = list_first(tokens); n != NULL; n = list_next(tokens, n)) {
        Token *tk = lnode_get(n);
        debug("%d: %s", tk->type, bdata(tk->data));
        Parse(parser, tk->type, tk, &settings);
    }

    Parse(parser, TKEOF, NULL, &settings);
    Parse(parser, 0, 0, &settings);
    tokens = NULL;

    debug("FINAL COUNT: %d", (int)hash_count(settings));
    return 0;

error:
    bdestroy(buffer);
    if(script) fclose(script);
    if(parser) ParseFree(parser, free);

    return -1;
}
