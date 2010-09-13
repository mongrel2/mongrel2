#ifndef _ast_h
#define _ast_h

#include <adt/list.h>
#include <adt/hash.h>
#include "parser.h"
#include "config_file.h"

typedef struct Pair {
    Token *key;
    void *value;
} Pair;

typedef struct Class {
    Token *ident;
    hash_t *params;
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
        hash_t *hash;
        Token *ident;
        Token *ref;
    } as;
} Value;

const char *Value_type_name(ValueType type);

Value *Value_create(ValueType type, void *data);
#define Value_is(V, T) ((V)->type == (VAL_##T))

typedef void (*ast_walk_cb)(hash_t *settings, Value *val);
typedef void (*ast_hash_walk_cb)(hash_t *settings, const char *name, Value *val);
void AST_walk(hash_t *settings, ast_walk_cb cb);
void AST_walk_list(hash_t *settings, list_t *data, ast_walk_cb cb);
void AST_walk_hash(hash_t *settings, Value *data, ast_hash_walk_cb cb);

#define Class_ident(C) ((C)->ident->data)
#define Class_params(C) ((C)->params)
#endif
