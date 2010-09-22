#ifndef _m2sh_token_h
#define _m2sh_token_h

typedef int TokenType;

typedef struct Token {
    TokenType type;
    bstring data;
} Token;

void Token_destroy(Token *tk);

Token *Token_create(TokenType type, const char *data, int length);

#endif
