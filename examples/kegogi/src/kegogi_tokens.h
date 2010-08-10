#ifndef _KEGOGI_TOKENS_H
#define _KEGOGI_TOKENS_H

#include <bstring.h>

typedef int TokenType;

typedef struct Token {
    TokenType type;
    bstring s1;
    bstring s2;
    bstring s3;
} Token;

typedef struct TokenList {
    int count;
    int size;
    Token tokens[0];
} TokenList;

char *blk2cstr(char *start, int len);

TokenList* TokenList_create(int size);
void TokenList_destroy(TokenList* list);
int TokenList_append0(TokenList** listP, TokenType type);
int TokenList_append1(TokenList** listP, TokenType type, bstring s1);
int TokenList_append2(TokenList** listP, TokenType type, bstring s1,
                      bstring s2);
int TokenList_append3(TokenList** listP, TokenType type, bstring s1, 
                      bstring s2, bstring s3);

#endif
