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
#include "adt/hash.h"
#include "dbg.h"
#include "request.h"
#include "tnetstrings.h"
#include "websocket.h"
#include "connection.h"

int MAX_HEADER_COUNT=0;
int MAX_DUPE_HEADERS=5;

static struct tagbstring CHUNKED = bsStatic("chunked");

void Request_init()
{
    MAX_HEADER_COUNT = Setting_get_int("limits.header_count", 128 * 10);
    log_info("MAX limits.header_count=%d", MAX_HEADER_COUNT);
}


static hnode_t *req_alloc_hash(void *notused)
{
    (void)notused;

    return (hnode_t *)malloc(sizeof(hnode_t));
}

static void req_free_hash(hnode_t *node, void *notused)
{
    (void)notused;

    bstrListDestroy((struct bstrList *)hnode_get(node));
    bdestroy((bstring)hnode_getkey(node));
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
    (void)at;
    (void)length;

    Request *req = (Request *)data;

    // extract chunked
    int chunked = 0;
    bstring te = Request_get(req, &HTTP_TRANSFER_ENCODING);
    if(te && !bstrcmp(te, &CHUNKED)) {
        chunked = 1;
    }

    // extract content_len
    const char *clen = bdata(Request_get(req, &HTTP_CONTENT_LENGTH));
    if(clen) {
        req->parser.content_len = atoi(clen);
    } else {
        if(chunked) {
            // if content-length missing, only assume indefinite length if
            //   chunked encoding is present
            req->parser.content_len = -1;
        } else {
            // otherwise 0 length
            req->parser.content_len = 0;
        }
    }

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

    if(hash_isfull(req->headers)) {
        log_err("Request had more than %d headers allowed by limits.header_count.",
                MAX_HEADER_COUNT);
    } else {
        bstring vstr = blk2bstr(value, vlen);
        bstring fstr = blk2bstr(field, flen);
        btolower(fstr);
        Request_set(req, fstr, vstr, 0);

        bdestroy(fstr); // we still own the key
    }
}

/**
 * The caller owns the key, and this function will duplicate it
 * if needed.  This function owns the value and will destroy it
 * if there's an error.
 */
void Request_set(Request *req, bstring key, bstring val, int replace)
{
    hnode_t *n = hash_lookup(req->headers, key);
    struct bstrList *val_list = NULL;
    int rc = 0;
    int i = 0;

    if(n == NULL) {
        // make a new bstring list to use as our storage
        val_list = bstrListCreate();
        rc = bstrListAlloc(val_list, MAX_DUPE_HEADERS);
        check(rc == BSTR_OK, "Couldn't allocate space for header values.");

        val_list->entry[0] = val;
        val_list->qty = 1;
        hash_alloc_insert(req->headers, bstrcpy(key), val_list);
    } else {
        val_list = hnode_get(n);
        check(val_list != NULL, "Malformed request, missing bstrlist in node. Tell Zed: %s=%s", bdata(key), bdata(val));

        if(replace) {
            // destroy ALL old ones and put this in their place
            for(i = 0; i < val_list->qty; i++) {
                bdestroy(val_list->entry[i]);
            }

            val_list->entry[0] = val;
            val_list->qty = 1;
        } else {
            check(val_list->qty < MAX_DUPE_HEADERS, 
                    "Header %s duplicated more than %d times allowed.", 
                    bdata(key), MAX_DUPE_HEADERS);

            val_list->entry[val_list->qty++] = val;
        }
    }

    return;

error:
    bdestroy(val);
    return;
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

    req->headers = hash_create(MAX_HEADER_COUNT, (hash_comp_t)bstrcmp, bstr_hash_fun);
    check_mem(req->headers);
    hash_set_allocator(req->headers, req_alloc_hash, req_free_hash, NULL);

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
    req->parser.json_sent = 0;
    req->parser.xml_sent = 0;
    req->ws_flags=0;
    bdestroy(req->new_header); req->new_header=NULL;
}

void Request_destroy(Request *req)
{
    if(req) {
        Request_nuke_parts(req);
        hash_free_nodes(req->headers);
        hash_destroy(req->headers);
        free(req);
    }
}


void Request_start(Request *req)
{
    assert(req && "NULL pointer error.");
    http_parser_init(&(req->parser));

    Request_nuke_parts(req);

    if(req->headers) {
        hash_free_nodes(req->headers);
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
    hnode_t *node = hash_lookup(req->headers, field);
    struct bstrList *vals = NULL;

    if(node == NULL) {
        return NULL;
    } else {
        vals = hnode_get(node);
        return vals->entry[0];
    }
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

static inline bstring json_escape(bstring in)
{
    if(in == NULL) return NULL;

    // Slightly better than the old solution.
    bstring vstr = bstrcpy(in);
    check_mem(vstr)
    
    int i;
    for(i = 0; i < blength(vstr); i++)
    {
        if(bchar(vstr,i) == '\\')
        {
            binsertch(vstr, i, 1, '\\');
            i++;
        }
        else if(bchar(vstr,i) == '"')
        {
            binsertch(vstr, i, 1, '\\');
            i++;
        }
    }

    return vstr;
error:
    return NULL;
}

struct tagbstring JSON_LISTSEP = bsStatic("\",\"");
struct tagbstring JSON_OBJSEP = bsStatic("\":\"");

// This chosen extremely arbitrarily. Certainly needs work.
static const int PAYLOAD_GUESS = 256;

static inline void B(bstring headers, const bstring k, const bstring v, int *first)
{
    if(v)
    {
        if(*first) {
            bcatcstr(headers, "\"");
            *first = 0;
        } else {
            bcatcstr(headers, ",\"");
        }
        bconcat(headers, k);
        bconcat(headers, &JSON_OBJSEP);

        bstring vstr = json_escape(v);
        bconcat(headers, vstr);
        bcatcstr(headers, "\"");

        bdestroy(vstr);
    }
}

static inline bstring request_determine_method(Request *req)
{
    if(Request_is_json(req)) {
        return &JSON_METHOD;
    } else if(Request_is_xml(req)) {
        return &XML_METHOD;
    } else {
        return req->request_method;
    }
}

bstring Request_to_tnetstring(Request *req, bstring uuid, int fd, const char *buf, size_t len, Connection *conn, hash_t *altheaders)
{
    tns_outbuf outbuf = {.buffer = NULL};
    bstring method = request_determine_method(req);
    check(method, "Impossible, got an invalid request method.");

    uint32_t id = Register_id_for_fd(fd);
    check_debug(id != UINT32_MAX, "Asked to generate a payload for a fd that doesn't exist: %d", fd);

    int header_start = tns_render_request_start(&outbuf);
    check(header_start != -1, "Failed to initialize outbuf.");

    if(altheaders != NULL) {
        check(tns_render_request_headers(&outbuf, altheaders) == 0,
                "Failed to render headers to a tnetstring.");
    } else {
        check(tns_render_request_headers(&outbuf, req->headers) == 0,
                "Failed to render headers to a tnetstring.");

        if(req->path) tns_render_hash_pair(&outbuf, &HTTP_PATH, req->path);
        if(req->version) tns_render_hash_pair(&outbuf, &HTTP_VERSION, req->version);
        if(req->uri) tns_render_hash_pair(&outbuf, &HTTP_URI, req->uri);
        if(req->query_string) tns_render_hash_pair(&outbuf, &HTTP_QUERY, req->query_string);
        if(req->fragment) tns_render_hash_pair(&outbuf, &HTTP_FRAGMENT, req->fragment);
        if(req->pattern) tns_render_hash_pair(&outbuf, &HTTP_PATTERN, req->pattern);
        /* TODO distinguish websocket with ws and wss? */
        if(conn->iob->use_ssl) {
            tns_render_hash_pair(&outbuf, &HTTP_URL_SCHEME, &HTTP_HTTPS);
        } else {
            tns_render_hash_pair(&outbuf, &HTTP_URL_SCHEME, &HTTP_HTTP);
        }

        tns_render_hash_pair(&outbuf, &HTTP_METHOD, method);
        bstring bremote = bfromcstr(conn->remote);
        tns_render_hash_pair(&outbuf, &HTTP_REMOTE_ADDR, bremote);
        if (bremote) {
            bdestroy(bremote);
        }
    }

    check(tns_render_request_end(&outbuf, header_start, uuid, id, Request_path(req)) != -1, "Failed to finalize request.");

    // header now owns the outbuf buffer
    bstring headers = tns_outbuf_to_bstring(&outbuf);
    bformata(headers, "%d:", len);
    bcatblk(headers, buf, len);
    bconchar(headers, ',');

    return headers;
error:
    if(outbuf.buffer) free(outbuf.buffer);
    return NULL;
}

bstring Request_to_payload(Request *req, bstring uuid, int fd, const char *buf, size_t len, Connection *conn, hash_t *altheaders)
{
    bstring headers = NULL;
    bstring result = NULL;
    int f = 1; // first header flag

    uint32_t id = Register_id_for_fd(fd);
    check_debug(id != UINT32_MAX, "Asked to generate a payload for a fd that doesn't exist: %d", fd);

    headers = bfromcstralloc(PAYLOAD_GUESS, "{");

    if(altheaders == NULL) {
        bcatcstr(headers, "\"");
        bconcat(headers, &HTTP_PATH);
        bconcat(headers, &JSON_OBJSEP);
        bconcat(headers, req->path);
        bconchar(headers, '"');
        f = 0;
    }

    hscan_t scan;
    hnode_t *i;
    if(altheaders != NULL) {
        hash_scan_begin(&scan, altheaders);
    } else {
        hash_scan_begin(&scan, req->headers);
    }

    for(i = hash_scan_next(&scan); i != NULL; i = hash_scan_next(&scan))
    {
        struct bstrList *val_list = hnode_get(i);
        bstring key = (bstring)hnode_getkey(i);

        if(val_list->qty > 1)
        {
            struct bstrList *escaped = bstrListCreate();
            bstrListAlloc(escaped, val_list->qty);
            escaped->qty = val_list->qty;

            int x = 0;
            for(x = 0; x < val_list->qty; x++) {
                escaped->entry[x] = json_escape(val_list->entry[x]);
            }

            bstring list = bjoin(escaped, &JSON_LISTSEP);
            if(f) {
                bcatcstr(headers, "\"");
                f = 0;
            } else {
                bcatcstr(headers, ",\"");
            }
            bconcat(headers, key);
            bcatcstr(headers, "\":[\"");
            bconcat(headers, list);
            bcatcstr(headers, "\"]");
            bdestroy(list);
            bstrListDestroy(escaped);
        }
        else
        {
            B(headers, key, val_list->entry[0], &f);
        }
    }

    if(altheaders == NULL) {
        // these come after so that if anyone attempts to hijack these somehow, most
        // hash algorithms languages have will replace the browser ones with ours

        if(Request_is_json(req)) {
            B(headers, &HTTP_METHOD, &JSON_METHOD, &f);
        } else if(Request_is_xml(req)) {
            B(headers, &HTTP_METHOD, &XML_METHOD, &f);
        } else {
            B(headers, &HTTP_METHOD, req->request_method, &f);
        }

        B(headers, &HTTP_VERSION, req->version, &f);
        B(headers, &HTTP_URI, req->uri, &f);
        B(headers, &HTTP_QUERY, req->query_string, &f);
        B(headers, &HTTP_FRAGMENT, req->fragment, &f);
        B(headers, &HTTP_PATTERN, req->pattern, &f);

        /* TODO websocket "ws" and "wss"? */
        if(conn->iob->use_ssl) {
            B(headers, &HTTP_URL_SCHEME, &HTTP_HTTPS, &f);
        } else {
            B(headers, &HTTP_URL_SCHEME, &HTTP_HTTP, &f);
        }

        bstring bremote = bfromcstr(conn->remote);
        B(headers, &HTTP_REMOTE_ADDR, bremote, &f);
        if (bremote) {
            bdestroy(bremote);
        }
    }

    bconchar(headers, '}');

    result = bformat("%s %d %s %d:%s,%d:", bdata(uuid), id, 
            bdata(Request_path(req)),
            blength(headers),
            bdata(headers),
            len);

    bdestroy(headers);

    bcatblk(result, buf, len);
    bconchar(result, ',');

    check(result, "Failed to construct payload result.");

    return result;

error:
    bdestroy(headers);
    bdestroy(result);

    return NULL;
}

void Request_set_relaxed(Request *req, int enabled)
{
    req->parser.uri_relaxed = (enabled ? 1 : 0);
}
