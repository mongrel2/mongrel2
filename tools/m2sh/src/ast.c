/**
 *
 * Copyright (c) 2010, Zed A. Shaw and Mongrel2 Project Contributors.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 * 
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 * 
 *     * Neither the name of the Mongrel2 Project, Zed A. Shaw, nor the names
 *       of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written
 *       permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "ast.h"
#include <stdlib.h>
#include <dbg.h>
#include <assert.h>
#include <tnetstrings_impl.h>

Value *Value_create(ValueType type, void *data) {
    Value *val = malloc(sizeof(Value));
    val->type = type;
    switch(val->type) {
        case VAL_QSTRING: val->as.string = data;
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
    "QSTRING", "NUMBER", "CLASS", "LIST", "HASH", 
    "IDENT", "REF"
};

const char *Value_type_name(ValueType type)
{
    return VALUE_NAMES[type];
}

Value *Value_resolve(tst_t *settings, Value *val)
{
    if(Value_is(val, REF)) {
        Pair *pair = tst_search(settings, bdata(val->as.ref->data), blength(val->as.ref->data));
        check(pair != NULL, "Couldn't find variable named: %s", bdata(val->as.ref->data));
        return Pair_value(pair);
    } else {
        return val;
    }

error:
    return NULL;
}

int AST_walk_list(tst_t *settings, list_t *data, ast_walk_cb cb)
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

struct tagbstring DEFAULT_ROOT = bsStatic("servers");

int AST_walk(tst_t *settings, ast_walk_cb cb)
{
    Pair *n = tst_search(settings, bdata(&DEFAULT_ROOT), blength(&DEFAULT_ROOT));
    check(n, "You didn't set a %s variable to say what servers you want.", 
            bdata(&DEFAULT_ROOT));

    Value *val = Pair_value(n);
    check(val->type == VAL_LIST, "servers variable should be a list of server configs to load.");

    return AST_walk_list(settings, val->as.list, cb);

error:
    return -1;
}

struct ASTScanData {
    tst_t *settings;
    ast_hash_walk_cb cb;
    int error;
};

void ast_hash_traverse_cb(void *value, void *data)
{
    // we want to *copy* this pair so that we can resolve it on the fly
    Pair pair = *((Pair *)value);
    struct ASTScanData *scan = data;

    // then we temporarily just swap it out for this callback if we resolve
    if(Value_is(pair.value, REF)) {
        pair.value = Value_resolve(scan->settings, pair.value);
    }

    int rc = scan->cb(scan->settings, &pair);
    scan->error = scan->error == 0 ? rc : scan->error;
}

int AST_walk_hash(tst_t *settings, Value *data, ast_hash_walk_cb cb)
{
    struct ASTScanData scan = {.settings = settings, .cb = cb, .error = 0};
    tst_traverse(data->as.hash, ast_hash_traverse_cb, &scan);
    return scan.error;
}


Value *AST_get(tst_t *settings, tst_t *fr, bstring name, ValueType type)
{
    Pair *pair = tst_search(fr, bdata(name), blength(name));
    check_debug(pair, "Couldn't find variable %s of type %s", bdata(name), Value_type_name(type));

    Value *val = Pair_value(pair);

    if(Value_is(val, REF)) {
        val = Value_resolve(settings, val);
        check(val, "Couldn't find variable %s of type %s",
            bdata(name), Value_type_name(type));
    }

    check(val->type == type, "Invalid type for %s, should be %s not %s",
            bdata(name), Value_type_name(type), Value_type_name(val->type));

    return val;

error:
    return NULL;
}


bstring AST_get_bstr(tst_t *settings, tst_t *fr, bstring name, ValueType type)
{
    Value *val = AST_get(settings, fr, name, type);

    check(val != NULL, "The server is missing the '%s' variable.", 
            bdata(name));

    return val->as.string->data;

error:
    return NULL;
}

const char *AST_str(tst_t *settings, tst_t *fr, const char *name, TokenType type)
{
    bstring key = bfromcstr(name);
    bstring val = AST_get_bstr(settings, fr, key, type);
    bdestroy(key);
    return bdata(val);
}

static void Class_destroy(Class *cls)
{
    Token_destroy(cls->ident);
    AST_destroy(cls->params);
    free(cls);
}


static void AST_destroy_value(Value *val);

static int AST_destroy_list(list_t *data)
{
    lnode_t *n = NULL;

    for(n = list_first(data); n != NULL; n = list_next(data, n)) {
        AST_destroy_value((Value *)lnode_get(n));
    }

    list_destroy_nodes(data);
    list_destroy(data);
    return 0;
}

static void AST_destroy_value(Value *val)
{
    switch(val->type) {
        case VAL_QSTRING: Token_destroy(val->as.string);
            break;
        case VAL_NUMBER: Token_destroy(val->as.number);
            break;
        case VAL_CLASS: Class_destroy(val->as.cls);
            break;
        case VAL_LIST: AST_destroy_list(val->as.list);
            break;
        case VAL_HASH: AST_destroy(val->as.hash);
            break;
        case VAL_IDENT: Token_destroy(val->as.ident);
            break;
        case VAL_REF: Token_destroy(val->as.ref);
            break;
        default:
            log_err("Unknown value type ID: %d. Tell Zed.", val->type);
    }

    free(val);
}

static void AST_destroy_cb(void *value, void *data)
{
    Pair *pair = (Pair *)value;

    AST_destroy_value(Pair_value(pair));
    Token_destroy(pair->key);
    free(pair);
}

void AST_destroy(tst_t *settings)
{
    tst_traverse(settings, AST_destroy_cb, NULL);
    tst_destroy(settings);
}


struct TNSScanData {
    tst_t *settings;
    tns_value_t *data;
    int error;
};

void AST_tns_convert_cb(void *v, void *data);

tns_value_t *AST_tns_convert_hash(tst_t *settings, Value *filter_settings)
{
    tns_value_t *filter_tns = tns_new_dict();

    struct TNSScanData data = {
        .settings = settings,
        .data = filter_tns,
        .error = 0
    };

    tst_traverse(filter_settings->as.hash, AST_tns_convert_cb, &data);
    check(!data.error, "Failed to convert hash in filter settings.");

    return filter_tns;
error:

    if(filter_tns) tns_value_destroy(filter_tns);
    return NULL;
}

tns_value_t *AST_tns_convert_value(tst_t *settings, Value *val)
{
    lnode_t *cur = NULL;
    tns_value_t *res = NULL;

    switch(val->type) {
        case VAL_QSTRING:
            res = tns_value_create(tns_tag_string);
            res->value.string = bstrcpy(val->as.string->data);
            break;

        case VAL_NUMBER:
            res = tns_new_integer(strtol(
                        bdatae(val->as.number->data, "0"), NULL, 10));
            break;

        case VAL_LIST:
            res = tns_new_list();
            for(cur = list_first(val->as.list);
                    cur != NULL; cur = list_next(val->as.list, cur)) 
            {
                tns_value_t *el = AST_tns_convert_value(settings, (Value *)lnode_get(cur));
                check(el != NULL, "Failed to convert an element of the list.");
                tns_add_to_list(res, el);
            }
            break;

        case VAL_REF:
            val = Value_resolve(settings, val);
            res = AST_tns_convert_value(settings, val);
            break;

        case VAL_HASH:
            res = AST_tns_convert_hash(settings, val);
            break;
        default:
            sentinel("Unsupported type in Filter settings: %s", Value_type_name(val->type));
    }

    return res;

error: // fallthrough
    return NULL;
}

void AST_tns_convert_cb(void *v, void *data)
{
    Pair *pair = v;
    struct TNSScanData *scan = data;
    bstring name = Pair_key(pair);
    Value *val = Pair_value(pair);
    check(val, "Error loading filter Setting %s", bdata(Pair_key(pair)));

    tns_value_t *value = AST_tns_convert_value(scan->settings, val);
    check(value != NULL, "Failed to convert filter setting value.");

    tns_value_t *key = tns_value_create(tns_tag_string);
    key->value.string = bstrcpy(name);

    tns_add_to_dict(scan->data, key, value);

    return;

error:
    scan->error = 1;
    tns_value_destroy(scan->data);
    scan->data = NULL;
}
