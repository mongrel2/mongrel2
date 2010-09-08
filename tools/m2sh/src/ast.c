#include "ast.h"
#include <stdlib.h>
#include <dbg.h>

void Value_dump(hash_t *settings, Value *val);
void AST_dump_list(hash_t *settings, list_t *data);
void AST_dump_hash(hash_t *settings, const char *name, hash_t *data);
void AST_walk(hash_t *settings);

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

void Value_dump(hash_t *settings, Value *val)
{
    hnode_t *n = NULL;

    switch(val->type) {
        case VAL_QSTRING:
            debug("\tSTRING: %s", bdata(val->as.string->data));
            break;
        case VAL_PATTERN:
            debug("\tPATTERN: %s", bdata(val->as.pattern->data));
            break;
        case VAL_NUMBER:
            debug("\tNUMBER: %s", bdata(val->as.number->data));
            break;
        case VAL_CLASS:
            debug(">>CLASS: %s", bdata(val->as.cls->ident->data));
            AST_dump_hash(settings, "CLASS", val->as.cls->params);
            debug("<<CLASS: %s", bdata(val->as.cls->ident->data));
            break;
        case VAL_LIST:
            debug(">>LIST:");
            AST_dump_list(settings, val->as.list);
            debug("<<LIST:");
            break;
        case VAL_HASH:
            debug(">>HASH");
            AST_dump_hash(settings, "HASH", val->as.hash);
            debug("<<HASH");
            break;
        case VAL_IDENT:
            debug("\tIDENT: %s", bdata(val->as.ident->data));
            break;
        case VAL_REF:
            debug("** REF: %s",  bdata(val->as.ref->data));
            n = hash_lookup(settings, bdata(val->as.ref->data));
            check(n != NULL, "Couldn't find variable named: %s", bdata(val->as.ref->data));
            Value_dump(settings, hnode_get(n));
            break;
    }

error:
    return;
}

void AST_dump_list(hash_t *settings, list_t *data)
{
    lnode_t *n = NULL;

    for(n = list_first(data); n != NULL; n = list_next(data, n)) {
        Value_dump(settings, (Value *)lnode_get(n));
    }
}

void AST_dump_hash(hash_t *settings, const char *name, hash_t *data)
{
    debug("%s: %d elements", name, (int)hash_count(data));
    hscan_t s;
    hnode_t *n = NULL;
    hash_scan_begin(&s, data);
    Value *val = NULL;

    while((n = hash_scan_next(&s)) != NULL) {
        val = (Value *)hnode_get(n);
        debug("KEY: %s, type: %d", (const char *)hnode_getkey(n), val->type);
        Value_dump(settings, val);
    }
}


void AST_walk(hash_t *settings)
{
    hnode_t *n = hash_lookup(settings, "servers");
    check(n, "You didn't set a servers variable to say what servers you want.");

    Value *val = hnode_get(n);
    check(val->type == VAL_LIST, "servers variable should be a list.");

    AST_dump_list(settings, val->as.list);
error:
    return;
}
