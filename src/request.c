#define _XOPEN_SOURCE 1

#include <request.h>
#include <dbg.h>
#include <stdlib.h>
#include <assert.h>
#include <adt/dict.h>
#include <string.h>
#include <headers.h>

#include <time.h>

enum {
    MAX_HEADER_COUNT=128 * 10
};


static dnode_t *req_alloc_dict(void *notused)
{
    return (dnode_t *)calloc(sizeof(dnode_t), 1);
}

static void req_free_dict(dnode_t *node, void *notused)
{
    bdestroy((bstring)dnode_get(node));
    bdestroy((bstring)dnode_getkey(node));
    free(node);
}


static void request_method_cb(void *data, const char *at, size_t length)
{
    Request *req = (Request *)data;
    req->request_method = blk2bstr(at, length);
}

static void fragment_cb(void *data, const char *at, size_t length)
{
    Request *req = (Request *)data;
    req->fragment = blk2bstr(at, length);
}

static void http_version_cb(void *data, const char *at, size_t length)
{
    Request *req = (Request *)data;
    req->version = blk2bstr(at, length);
}


static void header_done_cb(void *data, const char *at, size_t length)
{
    Request *req = (Request *)data;

    const char *clen = bdata(Request_get(req, &HTTP_CONTENT_LENGTH));

    if(clen) req->parser.content_len = atoi(clen);
        
    // TODO: do something else here like verify the request or call filters
}

static void uri_cb(void *data, const char *at, size_t length)
{
    Request *req = (Request *)data;
    req->uri = blk2bstr(at, length);
}

static void path_cb(void *data, const char *at, size_t length)
{
    Request *req = (Request *)data;
    req->path = blk2bstr(at, length);
}

static void query_string_cb(void *data, const char *at, size_t length)
{
    Request *req = (Request *)data;
    req->query_string = blk2bstr(at, length);
}

static void header_field_cb(void *data, const char *field, size_t flen,
        const char *value, size_t vlen)
{
    Request *req = (Request *)data;
    dict_alloc_insert(req->headers, blk2bstr(field, flen), blk2bstr(value, vlen));
}


Request *Request_create()
{
    Request *req = calloc(sizeof(Request), 1);
    check(req, "Out of Memory.");

    req->parser.http_field = header_field_cb;
    req->parser.request_method = request_method_cb;
    req->parser.request_uri = uri_cb;
    req->parser.fragment = fragment_cb;
    req->parser.request_path = path_cb;
    req->parser.query_string = query_string_cb;
    req->parser.http_version = http_version_cb;
    req->parser.header_done = header_done_cb;

    req->headers = dict_create(MAX_HEADER_COUNT, (dict_comp_t)bstricmp);
    check(req->headers, "Out of Memory");
    dict_set_allocator(req->headers, req_alloc_dict, req_free_dict, NULL);
    dict_allow_dupes(req->headers);

    req->parser.data = req;  // for the http callbacks

    return req;

error:
    Request_destroy(req);
    return NULL;
}

inline void Request_nuke_parts(Request *req)
{
    bdestroy(req->request_method); req->request_method = NULL;
    bdestroy(req->version); req->version = NULL;
    bdestroy(req->uri); req->uri = NULL;
    bdestroy(req->path); req->path = NULL;
    bdestroy(req->query_string); req->query_string = NULL;
    bdestroy(req->fragment); req->fragment = NULL;
}

void Request_destroy(Request *req)
{
    if(req) {
        Request_nuke_parts(req);
        dict_free_nodes(req->headers);
        dict_destroy(req->headers);
        free(req);
    }
}


void Request_start(Request *req)
{
    assert(req && "NULL pointer error.");
    http_parser_init(&(req->parser));

    Request_nuke_parts(req);

    if(req->headers) {
        dict_free_nodes(req->headers);
    }
}

int Request_parse(Request *req, char *buf, size_t nread, size_t *out_nparsed)
{
    assert(req && "NULL pointer error.");

    *out_nparsed = http_parser_execute(&(req->parser), buf, nread, *out_nparsed);
    
    int finished =  http_parser_finish(&(req->parser));

    return finished;
}


void Request_dump(Request *req)
{
    dnode_t *node = NULL;

    if(Request_is_socket(req)) {
        debug("FLASH SOCKET REQUEST of LENGTH: %d", (int)req->parser.body_start);
        return;
    } else if(Request_is_json(req)) {
        debug("JSON REQUEST of LENGTH: %d", (int)req->parser.body_start);
    } else if(Request_is_http(req)) {
        debug("HTTP REQUEST of LENGTH: %d ***********", (int)req->parser.body_start);
        debug("PATH: %s", bdata(req->path));
    } else {
        sentinel("UNKNOWN REQUEST TYPE, TELL ZED.");
    }

    for(node = dict_first(req->headers); node != NULL; node = dict_next(req->headers, node)) {
        bstring key = (bstring)dnode_getkey(node);
        bstring value = (bstring)dnode_get(node);

        debug("%s: %s", bdata(key), bdata(value));
    }

error:
    return;
}

bstring Request_get(Request *req, bstring field)
{
    dnode_t *node = dict_lookup(req->headers, field);

    return node == NULL ? NULL : (bstring)dnode_get(node);
}


int Request_get_date(Request *req, bstring field, const char *format)
{
    struct tm tm_val;
    bstring value = Request_get(req, field);

    if(value) {
        memset(&tm_val, 0, sizeof(struct tm));
        if(strptime(bdata(value), format, &tm_val) == NULL) {
            return 0;
        } else {
            return (int)mktime(&tm_val);
        }
    } else {
        return 0;
    }
}


#define B(K, V) if((V) != NULL) bformata(headers, ",\"%s\":\"%s\"", bdata(K), bdata(V))

bstring Request_to_payload(Request *req, bstring uuid, int fd, const char *buf, size_t len)
{
    bstring headers = bformat("{\"%s\":\"%s\"", bdata(&HTTP_PATH), bdata(req->path));
    bstring result = NULL;
    dnode_t *i = NULL;

    if(Request_is_json(req)) {
        B(&HTTP_METHOD, &JSON_METHOD);
    } else {
        B(&HTTP_METHOD, req->request_method);
    }

    B(&HTTP_VERSION, req->version);
    B(&HTTP_URI, req->uri);
    B(&HTTP_QUERY, req->query_string);
    B(&HTTP_FRAGMENT, req->fragment);

    for(i = dict_first(req->headers); i != NULL; i = dict_next(req->headers, i))
    {
        B((bstring)dnode_getkey(i), (bstring)dnode_get(i));
    }

    bconchar(headers, '}');

    result = bformat("%s %d %s %d:%s,%d:", bdata(uuid), fd, 
            bdata(Request_path(req)),
            blength(headers), bdata(headers), len);

    bcatblk(result, buf, len);
    bconchar(result, ',');

    check(result, "Failed to construct payload result.");

    bdestroy(headers);
    return result;

error:
    bdestroy(headers);
    return NULL;
}
