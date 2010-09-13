#ifndef _config_file_h
#define _config_file_h

#include <bstring.h>
#include <adt/hash.h>

typedef int TokenType;

typedef struct Token {
    TokenType type;
    bstring data;
} Token;

void Token_destroy(Token *tk);

hash_t *Parse_config_string(bstring content);

hash_t *Parse_config_file(const char *path);

typedef struct ParserState {
    hash_t *settings;
    int line_number;
    int error;
} ParserState;

#endif
