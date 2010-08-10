#include "kegogi_tokens.h"

#include <stdlib.h>

#include <dbg.h>

TokenList* TokenList_create(int size) {
    TokenList *list = malloc(sizeof(TokenList) + (sizeof(Token) * size));
    if(!list) return NULL;
    
    list->size = size;
    list->count = 0;

    return list;
}

static int TokenList_resize(TokenList** listP, int new_size) {
    if(!listP || !(*listP)) return -1;
    TokenList *new = realloc((*listP), 
                             sizeof(TokenList) + (sizeof(Token) * new_size));
    check_mem(new);

    new->size = new_size;
    *listP = new;

    return 0;

error:
    TokenList_destroy(*listP);
    return -1;
}

void TokenList_destroy(TokenList* list) {
    if(list) {
        int i;
        for(i = 0; i < list->count; i++) {
            if(list->tokens[i].s1) free(list->tokens[i].s1);
            if(list->tokens[i].s2) free(list->tokens[i].s2);
            if(list->tokens[i].s3) free(list->tokens[i].s3);
        }
        free(list);
    }
    
}

int TokenList_append0(TokenList** listP, TokenType type) {
    return TokenList_append3(listP, type, 0, 0, 0);
}

int TokenList_append1(TokenList** listP, TokenType type, bstring s1) {
    return TokenList_append3(listP, type, s1, 0, 0);
}
int TokenList_append2(TokenList** listP, TokenType type, bstring s1, bstring s2) {
    return TokenList_append3(listP, type, s1, s2, 0);
}

int TokenList_append3(TokenList** listP, TokenType type, bstring s1, bstring s2,
                      bstring s3) {
    check(listP != NULL && *listP != NULL, "Invalid pointer passed to append");
    
    TokenList *list = *listP;
    if(list->count + 1 > list->size) {
        int rc = TokenList_resize(listP, list->size * 2);
        check(rc == 0, "Failed to resize TokenList");
    }

    list = *listP; // This may have changed in grow

    list->count++;
    int idx = list->count - 1;
    
    list->tokens[idx].type = type;
    list->tokens[idx].s1 = s1;
    list->tokens[idx].s2 = s2;
    list->tokens[idx].s3 = s3;

    return 0;
error:
    return -1;
}
