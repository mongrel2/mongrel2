#include <stdlib.h>
#include <dbg.h>
#include "config.h"
#include <assert.h>

void Token_destroy(Token *tk)
{
    if(tk) {
        bdestroy(tk->data);
        free(tk);
    }
}

