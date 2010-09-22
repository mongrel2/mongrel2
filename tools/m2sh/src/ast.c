#include "ast.h"
#include <stdlib.h>
#include <dbg.h>
#include <assert.h>


Value *Value_create(ValueType type, void *data) {
    Value *val = malloc(sizeof(Value));
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

int AST_walk_list(hash_t *settings, list_t *data, ast_walk_cb cb)
{
    lnode_t *n = NULL;
    int rc = 0;

    for(n = list_first(data); n != NULL; n = list_next(data, n)) {
        Value *ref = lnode_get(n);
        check(ref, "List got a NULL value in it. Huh?");

        Value *found = Value_resolve(settings, ref);

        check(found, "Invalid reference: %s", bdata(ref->as.ref->data));
        rc = cb(settings, found);
        check_debug(rc == 0, "Failure processing config file. Aborting.");
    }

    return 0;

error:
    return -1;
}

int AST_walk(hash_t *settings, ast_walk_cb cb)
{
    hnode_t *n = hash_lookup(settings, "servers");
    check(n, "You didn't set a servers variable to say what servers you want.");

    Value *val = hnode_get(n);
    check(val->type == VAL_LIST, "servers variable should be a list of server configs to load.");

    return AST_walk_list(settings, val->as.list, cb);

error:
    return -1;
}

int AST_walk_hash(hash_t *settings, Value *data, ast_hash_walk_cb cb)
{
    hscan_t s;
    hnode_t *n = NULL;
    hash_scan_begin(&s, data->as.hash);
    Value *val = NULL;
    int rc = 0;

    while((n = hash_scan_next(&s)) != NULL) {
        val = hnode_get(n);
        rc = cb(settings, hnode_getkey(n), Value_resolve(settings, val));
        check_debug(rc == 0, "Failed processing config file. Aborting.");
    }

    return 0;

error:
    return -1;
}


Value *AST_get(hash_t *settings, hash_t *fr, const char *name, ValueType type)
{
    hnode_t *hn = hash_lookup(fr, name);
    check_debug(hn, "Variable %s not found, assuming not given.", name);

    Value *val = hnode_get(hn);
    if(Value_is(val, REF)) {
        val = Value_resolve(settings, val);
    }

    check(val->type == type, "Invalid type for %s, should be %s not %s",
            name, Value_type_name(type), Value_type_name(val->type));

    return val;

error:
    return NULL;
}


bstring AST_get_bstr(hash_t *settings, hash_t *fr, const char *name, ValueType type)
{
    Value *val = AST_get(settings, fr, name, type);
    check(val != NULL, "Variable %s is expected to be a %s but it's not.", name, Value_type_name(type));

    return val->as.string->data;

error:
    return NULL;
}


static void Class_destroy(hash_t *settings, Class *cls)
{
    Token_destroy(cls->ident);
    AST_destroy(cls->params);
    free(cls);
}


static int AST_destroy_cb(hash_t *settings, Value *val);

static int AST_destroy_list(hash_t *settings, list_t *data)
{
    lnode_t *n = NULL;

    for(n = list_first(data); n != NULL; n = list_next(data, n)) {
        Value *ref = lnode_get(n);
        AST_destroy_cb(settings, ref);
    }

    list_destroy_nodes(data);
    list_destroy(data);
    return 0;
}

static int AST_destroy_cb(hash_t *settings, Value *val)
{
    switch(val->type) {
        case VAL_QSTRING: Token_destroy(val->as.string);
            break;
        case VAL_PATTERN: Token_destroy(val->as.pattern);
            break;
        case VAL_NUMBER: Token_destroy(val->as.number);
            break;
        case VAL_CLASS: Class_destroy(settings, val->as.cls);
            break;
        case VAL_LIST: AST_destroy_list(settings, val->as.list);
            break;
        case VAL_HASH: AST_destroy(val->as.hash);
            break;
        case VAL_IDENT: Token_destroy(val->as.ident);
            break;
        case VAL_REF: Token_destroy(val->as.ref);
            break;
        default:
            log_err("Unknown value type: %d", val->type);
    }

    free(val);

    return 0;
}

void AST_destroy(hash_t *settings)
{
    hscan_t s;
    hnode_t *n = NULL;
    hash_scan_begin(&s, settings);
    Value *val = NULL;

    while((n = hash_scan_next(&s)) != NULL) {
        val = hnode_get(n);
        AST_destroy_cb(settings, val);
        hash_scan_delfree(settings, n);
    }
    
    hash_destroy(settings);
}


