#include <request.h>
#include <dbg.h>
#include <stdlib.h>
#include <assert.h>
#include <adt/dict.h>
#include <string.h>

enum {
    MAX_HEADER_COUNT=128
};

static void add_named_param(dict_t *req, const char *name, const char *at, size_t length)
{
    char *value = strndup(at, length);
    dict_alloc_insert(req, name, value);
}


static dnode_t *req_alloc_dict(void *notused)
{
    return (dnode_t *)calloc(sizeof(dnode_t), 1);
}

static void req_free_dict(dnode_t *node, void *notused)
{
    free(dnode_get(node));
    free((void *)dnode_getkey(node));
    free(node);
}


static void request_method_cb(void *data, const char *at, size_t length)
{
    add_named_param((dict_t *)data, strdup("REQUEST_METHOD"), at, length);
}

static void fragment_cb(void *data, const char *at, size_t length)
{
    add_named_param((dict_t *)data, strdup("FRAGMENT"), at, length);
}

static void http_version_cb(void *data, const char *at, size_t length)
{
    add_named_param((dict_t *)data, strdup("VERSION"), at, length);
}

static void header_done_cb(void *data, const char *at, size_t length)
{
    dict_verify((dict_t *)data);
}

static void uri_cb(void *data, const char *at, size_t length)
{
    add_named_param((dict_t *)data, strdup("URI"), at, length);
}

static void path_cb(void *data, const char *at, size_t length)
{
    add_named_param((dict_t *)data, strdup("PATH"), at, length);
}

static void query_string_cb(void *data, const char *at, size_t length)
{
    add_named_param((dict_t *)data, strdup("QUERY_STRING"), at, length);
}

static void header_field_cb(void *data, const char *field, size_t flen,
        const char *value, size_t vlen)
{
    dict_t *dict = (dict_t *)data;
    char *key = strndup(field, flen);
    add_named_param((dict_t *)data, key, value, vlen);
}


http_parser *Request_create()
{
    http_parser *parser = calloc(sizeof(http_parser), 1);
    check(parser, "Out of Memory.");

    parser->http_field = header_field_cb;
    parser->request_method = request_method_cb;
    parser->request_uri = uri_cb;
    parser->fragment = fragment_cb;
    parser->request_path = path_cb;
    parser->query_string = query_string_cb;
    parser->http_version = http_version_cb;
    parser->header_done = header_done_cb;

    parser->data = dict_create(MAX_HEADER_COUNT, (dict_comp_t)strcmp);
    dict_set_allocator(parser->data, req_alloc_dict, req_free_dict, NULL);
    dict_allow_dupes(parser->data);

    return parser;

error:
    Request_destroy(parser);
    return NULL;
}


void Request_destroy(http_parser *parser)
{
    if(parser) {
        dict_free_nodes(parser->data);
        dict_destroy(parser->data);
        free(parser);
    }
}


void Request_start(http_parser *parser)
{
    assert(parser && "NULL pointer error.");
    http_parser_init(parser);
}

int Request_parse(http_parser *parser, char *buf, size_t nread,
        size_t *out_nparsed)
{
    assert(parser && "NULL pointer error.");

    *out_nparsed = http_parser_execute(parser, buf, nread, 0);
    
    int finished =  http_parser_finish(parser);

    return finished;
}


void Request_dump(http_parser *parser)
{
    dict_t *req = (dict_t *)parser->data;
    dnode_t *node = NULL;

    if(parser->socket_started) {
        debug("FLASH SOCKET REQUEST of LENGTH: %d", parser->body_start);
        return;
    } else if(parser->json_sent) {
        debug("JSON REQUEST of LENGTH: %d", parser->body_start);
        return;
    } else {
        debug("HTTP REQUEST\n---------");
    }

    for(node = dict_first(req); node != NULL; node = dict_next(req, node)) {
        debug("%s: %s", dnode_getkey(node), dnode_get(node));
    }
    
    debug("REQUEST HEADER LENGTH: %d\n------------", parser->body_start);
}

const char *Request_get(http_parser *parser, const char *field)
{
    dict_t *req = (dict_t *)parser->data;

    dnode_t *node = dict_lookup(req, field);

    if(node) {
        return (const char *)dnode_get(node);
    } else {
        return NULL;
    }
}
