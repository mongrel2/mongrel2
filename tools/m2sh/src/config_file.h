#ifndef _m2sh_config_file_h
#define _m2sh_config_file_h

#include <bstring.h>
#include <adt/tst.h>
#include "token.h"

tst_t *Parse_config_string(bstring content);

tst_t *Parse_config_file(const char *path);

typedef struct ParserState {
    tst_t *settings;
    int line_number;
    int error;
} ParserState;

struct Pair;
struct Value;

int Dir_load(tst_t *settings, tst_t *params);

int Handler_load(tst_t *settings, tst_t *params);

int Proxy_load(tst_t *settings, tst_t *params);

int Settings_load(tst_t *settings, struct Pair *pair);

int Route_load(tst_t *settings, struct Pair *pair);

int Host_load(tst_t *settings, struct Value *val);

int Server_load(tst_t *settings, struct Value *val);

int Config_load(const char *config_file, const char *db_file);

#endif
