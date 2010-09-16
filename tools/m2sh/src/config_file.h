#ifndef _m2sh_config_file_h
#define _m2sh_config_file_h

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

struct Value;

int Dir_load(hash_t *settings, hash_t *params);

int Handler_load(hash_t *settings, hash_t *params);

int Proxy_load(hash_t *settings, hash_t *params);

int Settings_load(hash_t *settings, const char *name, struct Value *val);

int Route_load(hash_t *settings, const char *name, struct Value *val);

int Host_load(hash_t *settings, struct Value *val);

int Server_load(hash_t *settings, struct Value *val);

int Config_load(const char *config_file, const char *db_file);

#endif
