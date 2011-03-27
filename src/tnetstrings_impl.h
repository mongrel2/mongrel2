#ifndef _tnetstrings_impl_h
#define _tnetstrings_impl_h

int MAX_HASH_COUNT=128 * 10;


/** Helper functions for the tns code to work with internal data structures. */

void tns_value_destroy(tns_value_t *value)
{
    list_t *L = value->value.list;
    lnode_t *n = NULL;

    switch(value->type) {
        case tns_tag_bool:
            break;
        case tns_tag_dict:
            hash_free_nodes(value->value.dict);
            hash_free(value->value.dict);
            break;
        case tns_tag_list:
            for(n = list_last(L); n != NULL; n = list_prev(L, n)) {
                tns_value_destroy(lnode_get(n));
            }

            list_destroy_nodes(value->value.list);
            list_destroy(value->value.list);
            break;
        case tns_tag_null:
            break;
        case tns_tag_number:
            break;
        case tns_tag_string:
            bdestroy(value->value.string);
            break;
        default:
            sentinel("Invalid type given: '%c'", value->type);
    }

error: // fallthrough
    free(value);
    return;
}

void tns_hnode_free(hnode_t *node, void *notused)
{
    bdestroy((bstring)hnode_getkey(node));
    tns_value_destroy(hnode_get(node));
    free(node);
}

hnode_t *tns_hnode_alloc(void *notused)
{
    return malloc(sizeof(hnode_t));
}


//  Functions to introspect the type of a data object.
static inline tns_type_tag tns_get_type(void *val)
{
    tns_value_t *t = (tns_value_t *)val;
    return t->type;
}

//  Functions for parsing and rendering primitive datatypes.
static inline void *tns_parse_string(const char *data, size_t len)
{
    tns_value_t *t = calloc(sizeof(tns_value_t), 1);
    t->type = tns_tag_string;
    t->value.string = blk2bstr(data, len);
    return t;
}

static inline int tns_render_string(void *val, tns_outbuf *outbuf)
{
    tns_value_t *t = (tns_value_t *)val;
    assert(t->type == tns_tag_string && "Value is not a string.");
    return tns_outbuf_rputs(outbuf, bdata(t->value.string), blength(t->value.string));
}

static inline void *tns_parse_integer(const char *data, size_t len)
{
    tns_value_t *t = calloc(sizeof(tns_value_t), 1);
    t->type = tns_tag_number;
    t->value.number = strtol(data, NULL, 10);
    return t;
}

static inline int tns_render_number(void *val, tns_outbuf *outbuf)
{
    tns_value_t *t = (tns_value_t *)val;
    char out[120] = {0};

    assert(t->type == tns_tag_bool && "Value is not a string.");

    int rc = snprintf(out, 119, "%ld", t->value.number);
    check(rc != -1 && rc <= 119, "Failed to generate number.");

    out[119] = '\0'; // safety since snprintf might not do this

    return tns_outbuf_rputs(outbuf, out, rc);

error:
    return -1;
}

static inline int tns_render_bool(void *val, tns_outbuf *outbuf)
{
    tns_value_t *t = (tns_value_t *)val;
    assert(t->type == tns_tag_bool && "Value is not a string.");

    if(t->value.bool) {
        return tns_outbuf_rputs(outbuf, "true", 4);
    } else {
        return tns_outbuf_rputs(outbuf, "false", 5);
    }
}


//  Constructors to get constant primitive datatypes.
static inline void *tns_get_null(void)
{
    tns_value_t *t = calloc(sizeof(tns_value_t), 1);
    t->type = tns_tag_null;
    return t;
}

static inline void *tns_get_true(void)
{
    tns_value_t *t = calloc(sizeof(tns_value_t), 1);
    t->type = tns_tag_bool;
    t->value.bool = 1;
    return t;
}

static inline void *tns_get_false(void)
{
    tns_value_t *t = calloc(sizeof(tns_value_t), 1);
    t->type = tns_tag_bool;
    t->value.bool = 0;
    return t;
}


//  Functions for manipulating compound datatypes.
static inline void *tns_new_dict(void)
{
    tns_value_t *val = calloc(sizeof(tns_value_t), 1);
    val->value.dict = hash_create(MAX_HASH_COUNT, (hash_comp_t)bstrcmp, bstr_hash_fun);
    hash_set_allocator(val->value.dict, tns_hnode_alloc, tns_hnode_free, NULL);
    val->type = tns_tag_dict;

    return val;
}

static inline void tns_free_dict(void* dict)
{
    // TODO: this will leak the contents, need to determine the contents
    // and then loop through to destroy or do an halloc or something
    hash_t *h = ((tns_value_t *)dict)->value.dict;

    hash_free_nodes(h);
    hash_destroy(h);
}

static inline int tns_add_to_dict(void *dict, void *key, void *item)
{
    hash_t *h = ((tns_value_t *)dict)->value.dict;
    tns_value_t *k = key;

    check(k->type == tns_tag_string, "Only strings can be keys.");
    check(hash_alloc_insert(h, k->value.string, item), "Failed to insert key into dict.");

    free(k); // no longer needed
    return 0;
error:
    return -1;
}

static inline int tns_render_dict(void *dict, tns_outbuf *outbuf)
{
    hash_t *h = ((tns_value_t *)dict)->value.dict;
    hscan_t hs;
    hnode_t *node;
    hash_scan_begin(&hs, h);
    tns_value_t key;

    while ((node = hash_scan_next(&hs))) {
        check(tns_render_value(hnode_get(node), outbuf) == 0, "Failed to render dict value.");

        key.type = tns_tag_string;
        key.value.string = (bstring)hnode_getkey(node);

        check(tns_render_value(&key, outbuf) == 0, "Failed to render dict key.");
    }

    return 0;
error:
    return -1;
}

static inline void *tns_new_list(void)
{
    tns_value_t *val = calloc(sizeof(tns_value_t), 1);

    val->value.list = list_create(LISTCOUNT_T_MAX);
    val->type = tns_tag_list;

    return val;
}

static inline void tns_free_list(void* list)
{
    list_t *L = (list_t *)list;
    lnode_t *n = NULL;

    for(n = list_last(L); n != NULL; n = list_prev(L, n)) {
        tns_value_destroy(lnode_get(n));
    }
    list_destroy_nodes(L);
    list_destroy(L);
}

static inline int tns_add_to_list(void *list, void *item)
{
    list_t *L = ((tns_value_t *)list)->value.list;

    lnode_t *node = lnode_create(item);
    list_append(L, node);

    return 0;
}

static inline int tns_render_list(void *list, tns_outbuf *outbuf)
{
    list_t *L = ((tns_value_t *)list)->value.list;
    lnode_t *n = NULL;

    for(n = list_last(L); n != NULL; n = list_prev(L, n)) {
        tns_value_t *val = lnode_get(n);
        check(tns_render_value(val, outbuf) == 0, "Failed to render list element.");
    }

    return 0;
error:
    return -1;
}


#endif
