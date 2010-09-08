#ifndef _ast_h
#define _ast_h

#include <adt/list.h>
#include <adt/hash.h>
#include "parser.h"
#include "config.h"

typedef struct Pair {
    Token *key;
    void *value;
} Pair;

typedef struct Class {
    Token *ident;
    hash_t *params;
} Class;

typedef enum ValueType {
    VAL_QSTRING, VAL_PATTERN, VAL_NUMBER, VAL_CLASS, VAL_LIST, VAL_HASH, VAL_IDENT, VAL_REF
} ValueType;

typedef struct Value {
    ValueType type;
    union {
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

Value *Value_create(ValueType type, void *data);

void AST_walk(hash_t *settings);

#endif
