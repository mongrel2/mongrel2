#include <stdlib.h>
#include <dbg.h>
#include "config_file.h"
#include <assert.h>

void Token_destroy(Token *tk)
{
    if(tk) {
        if(tk->data) {
            debug("DESTROY: %s", bdata(tk->data));
            bdestroy(tk->data);
        }
        free(tk);
    }
}

Token *Token_create(TokenType type, const char *data, int length)
{
    Token *tk = malloc(sizeof(Token));
    tk->type = type;
    tk->data = blk2bstr(data, length);
    debug("CREATE: %s", bdata(tk->data));
    return tk;
}
