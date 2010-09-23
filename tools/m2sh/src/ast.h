#ifndef _m2sh_ast_h
#define _m2sh_ast_h

#include <adt/list.h>
#include <adt/tst.h>
#include "parser.h"
#include "config_file.h"

struct Value;

typedef struct Pair {
    Token *key;
    struct Value *value;
} Pair;

#define Pair_key(P) ((P)->key->data)
#define Pair_value(P) ((P)->value)

typedef struct Class {
    Token *ident;
    tst_t *params;
} Class;

typedef enum ValueType {
    VAL_QSTRING=0, VAL_PATTERN, VAL_NUMBER, VAL_CLASS, VAL_LIST, VAL_HASH, VAL_IDENT, VAL_REF
} ValueType;

typedef struct Value {
    ValueType type;
    union {
        void *data;
        Token *string;
        Token *pattern;
        Token *number;
        Class *cls;
        list_t *list;
        tst_t *hash;
        Token *ident;
        Token *ref;
    } as;
} Value;

const char *Value_type_name(ValueType type);

Value *Value_create(ValueType type, void *data);
#define Value_is(V, T) ((V)->type == (VAL_##T))

typedef int (*ast_walk_cb)(tst_t *settings, Value *val);
typedef int (*ast_hash_walk_cb)(tst_t *settings, Pair *pair);

int AST_walk(tst_t *settings, ast_walk_cb cb);
int AST_walk_list(tst_t *settings, list_t *data, ast_walk_cb cb);
int AST_walk_hash(tst_t *settings, Value *data, ast_hash_walk_cb cb);

#define Class_ident(C) ((C)->ident->data)
#define Class_params(C) ((C)->params)

Value *AST_get(tst_t *settings, tst_t *fr, bstring name, ValueType type);
bstring AST_get_bstr(tst_t *settings, tst_t *fr, bstring name, ValueType type);
const char *AST_str(tst_t *settings, tst_t *fr, const char *name, TokenType type);

void AST_destroy(tst_t *settings);

#endif
