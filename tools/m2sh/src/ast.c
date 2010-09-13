#include "ast.h"
#include <stdlib.h>
#include <dbg.h>
#include <assert.h>


Value *Value_create(ValueType type, void *data) {
    Value *val = calloc(sizeof(Value), 1);
    val->type = type;
    switch(val->type) {
        case VAL_QSTRING: val->as.string = data;
            break;
        case VAL_PATTERN: val->as.pattern = data;
            break;
        case VAL_NUMBER: val->as.number = data;
            break;
        case VAL_CLASS: val->as.cls = data;
            break;
        case VAL_LIST: val->as.list = data;
            break;
        case VAL_HASH: val->as.hash = data;
            break;
        case VAL_IDENT: val->as.ident = data;
            break;
        case VAL_REF: val->as.ref = data;
            break;
        default:
            sentinel("Unknown value type: %d", val->type);
    }

    return val;

error:
    return val;
}

const char *VALUE_NAMES[] = {
    "QSTRING", "PATTERN", "NUMBER", "CLASS", "LIST", "HASH", 
    "IDENT", "REF"
};

const char *Value_type_name(ValueType type)
{
    return VALUE_NAMES[type];
}

Value *Value_resolve(hash_t *settings, Value *val)
{
    if(Value_is(val, REF)) {
        hnode_t *n = hash_lookup(settings, bdata(val->as.ref->data));
        check(n != NULL, "Couldn't find variable named: %s", bdata(val->as.ref->data));
        return hnode_get(n);
    } else {
        return val;
    }

error:
    return NULL;
}

void AST_walk_list(hash_t *settings, list_t *data, ast_walk_cb cb)
{
    lnode_t *n = NULL;

    for(n = list_first(data); n != NULL; n = list_next(data, n)) {
        Value *ref = lnode_get(n);
        Value *found = Value_resolve(settings, ref);
        check(found, "Invalid reference: %s", bdata(ref->as.ref->data));
        cb(settings, found);
    }

error:
    return;
}

void AST_walk(hash_t *settings, ast_walk_cb cb)
{
    hnode_t *n = hash_lookup(settings, "servers");
    check(n, "You didn't set a servers variable to say what servers you want.");

    Value *val = hnode_get(n);
    check(val->type == VAL_LIST, "servers variable should be a list.");

    AST_walk_list(settings, val->as.list, cb);

error:
    return;
}

void AST_walk_hash(hash_t *settings, Value *data, ast_hash_walk_cb cb)
{
    assert(Value_is(data, HASH) && "Invalid type, expected Hash.");

    hscan_t s;
    hnode_t *n = NULL;
    hash_scan_begin(&s, data->as.hash);
    Value *val = NULL;

    while((n = hash_scan_next(&s)) != NULL) {
        val = hnode_get(n);
        cb(settings, hnode_getkey(n), Value_resolve(settings, val));
    }
}
