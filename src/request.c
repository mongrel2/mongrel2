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

#define _XOPEN_SOURCE 1

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <time.h>

#include "setting.h"
#include "register.h"
#include "headers.h"
#include "adt/dict.h"
#include "dbg.h"
#include "request.h"

int MAX_HEADER_COUNT=0;


void Request_init()
{
    MAX_HEADER_COUNT = Setting_get_int("limits.header_count", 128 * 10);
    log_info("MAX limits.header_count=%d", MAX_HEADER_COUNT);
}


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

    // extract content_len
    const char *clen = bdata(Request_get(req, &HTTP_CONTENT_LENGTH));
    if(clen) req->parser.content_len = atoi(clen);

    // extract host header
    req->host = Request_get(req, &HTTP_HOST);
    int colon = bstrchr(req->host, ':');
    if(req->host) {
        req->host_name = colon > 0 ? bHead(req->host, colon) : bstrcpy(req->host);
    }
}

static void uri_cb(void *data, const char *at, size_t length)
{
    Request *req = (Request *)data;
    req->uri = blk2bstr(at, length);
}

static void path_cb(void *data, const char *at, size_t length)
{
    Request *req = (Request *)data;
    assert(req->path == NULL && "This should not happen, Tell Zed.");
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
    bstring vstr = blk2bstr(value, vlen);

    dict_alloc_insert(req->headers, blk2bstr(field, flen), vstr);
}


Request *Request_create()
{
    Request *req = calloc(sizeof(Request), 1);
    check_mem(req);

    req->parser.http_field = header_field_cb;
    req->parser.request_method = request_method_cb;
    req->parser.request_uri = uri_cb;
    req->parser.fragment = fragment_cb;
    req->parser.request_path = path_cb;
    req->parser.query_string = query_string_cb;
    req->parser.http_version = http_version_cb;
    req->parser.header_done = header_done_cb;

    req->headers = dict_create(MAX_HEADER_COUNT, (dict_comp_t)bstricmp);
    check_mem(req->headers);

    dict_set_allocator(req->headers, req_alloc_dict, req_free_dict, NULL);
    dict_allow_dupes(req->headers);

    req->parser.data = req;  // for the http callbacks

    return req;

error:
    Request_destroy(req);
    return NULL;
}

static inline void Request_nuke_parts(Request *req)
{
    bdestroy(req->request_method); req->request_method = NULL;
    bdestroy(req->version); req->version = NULL;
    bdestroy(req->uri); req->uri = NULL;
    bdestroy(req->path); req->path = NULL;
    bdestroy(req->query_string); req->query_string = NULL;
    bdestroy(req->fragment); req->fragment = NULL;
    bdestroy(req->host_name); req->host_name = NULL;

    // not owned by us: host, pattern, prefix
    req->pattern = NULL;
    req->prefix = NULL;
    req->host = NULL;

    req->status_code = 0;
    req->response_size = 0;
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


#define B(K, V) if((V) != NULL) { vstr = json_escape(V); bformata(headers, ",\"%s\":\"%s\"", bdata(K), bdata(vstr)); bdestroy(vstr); }

static struct tagbstring QUOTE_CHAR = bsStatic("\"");
static struct tagbstring QUOTE_REPLACE = bsStatic("\\\"");

static struct tagbstring BSLASH_CHAR = bsStatic("\\");
static struct tagbstring BSLASH_REPLACE = bsStatic("\\\\");

static inline bstring json_escape(bstring in)
{
    if(in == NULL) return NULL;

    // TODO: this isn't efficient at all, make it better
    bstring vstr = bstrcpy(in);

    // MUST GO IN THIS ORDER
    bfindreplace(vstr, &BSLASH_CHAR, &BSLASH_REPLACE, 0);
    bfindreplace(vstr, &QUOTE_CHAR, &QUOTE_REPLACE, 0);

    return vstr;
}

bstring Request_to_payload(Request *req, bstring uuid, int fd, const char *buf, size_t len)
{
    bstring headers = bformat("{\"%s\":\"%s\"", bdata(&HTTP_PATH), bdata(req->path));
    bstring result = NULL;
    dnode_t *i = NULL;
    int id = Register_id_for_fd(fd);
    bstring vstr = NULL; // used in the B macro

    check(id != -1, "Asked to generate a paylod for an fd that doesn't exist: %d", fd);

    if(Request_is_json(req)) {
        B(&HTTP_METHOD, &JSON_METHOD);
    } else if(Request_is_xml(req)) {
        B(&HTTP_METHOD, &XML_METHOD);
    } else {
        B(&HTTP_METHOD, req->request_method);
    }

    B(&HTTP_VERSION, req->version);
    B(&HTTP_URI, req->uri);
    B(&HTTP_QUERY, req->query_string);
    B(&HTTP_FRAGMENT, req->fragment);
    B(&HTTP_PATTERN, req->pattern);

    for(i = dict_first(req->headers); i != NULL; i = dict_next(req->headers, i))
    {
        B((bstring)dnode_getkey(i), (bstring)dnode_get(i));
    }

    bconchar(headers, '}');

    result = bformat("%s %d %s %d:%s,%d:", bdata(uuid), id, 
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
