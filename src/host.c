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

#include <host.h>
#include <string.h>
#include <dbg.h>
#include <assert.h>
#include <mem/halloc.h>
#include <dir.h>
#include "setting.h"

int MAX_HOST_NAME = 0;
int MAX_URL_PATH = 0;

void backend_destroy_cb(Route *r, RouteMap *map)
{
    if(r->data) {
        Backend *backend = (Backend *)r->data;
        switch(backend->type) {
            case BACKEND_HANDLER:
                Handler_destroy(backend->target.handler);
                break;
            case BACKEND_PROXY:
                Proxy_destroy(backend->target.proxy);
                break;
            case BACKEND_DIR:
                Dir_destroy(backend->target.dir);
                break;
            default:
                sentinel("Invalid backend type, Tell Zed.");
        }

        free(backend);

        r->data = NULL;
    }

error:
    return;
}

Host *Host_create(const char *name)
{
    if(!MAX_URL_PATH || !MAX_HOST_NAME) {
        MAX_URL_PATH = Setting_get_int("limits.url_path", 256);
        MAX_HOST_NAME = Setting_get_int("limits.host_name", 256);
        log_info("MAX limits.url_path=%d, limits.host_name=%d",
                MAX_URL_PATH, MAX_HOST_NAME);
    }

    Host *host = h_calloc(sizeof(Host), 1);
    check_mem(host);

    host->name = bfromcstr(name);
    check(blength(host->name) < MAX_HOST_NAME, "Host name too long (max %d): '%s'\n", 
            MAX_HOST_NAME, name);

    host->routes = RouteMap_create(backend_destroy_cb);
    check(host->routes, "Failed to create host route map for %s.", name);
    
    return host;

error:
    return NULL;
}


void Host_destroy(Host *host)
{
    if(host) {
        bdestroy(host->name);
        RouteMap_destroy(host->routes);
        h_free(host);
    }
}



int Host_add_backend(Host *host, const char *path, size_t path_len, BackendType type, void *target)
{
    Backend *backend = calloc(sizeof(Backend), 1);
    check_mem(backend);

    backend->type = type;

    if(type == BACKEND_HANDLER) {
        backend->target.handler = target;
    } else if(type == BACKEND_PROXY) {
        backend->target.proxy = target;
    } else if(type == BACKEND_DIR) {
        backend->target.dir = target;
    } else {
        sentinel("Invalid proxy type given: %d", type);
    }

    int rc = RouteMap_insert(host->routes, blk2bstr(path, path_len), backend);
    check(rc == 0, "Failed to insert into host %s route map.", bdata(host->name));

    return 0;
    
error:
    return -1;
}


Backend *Host_match_backend(Host *host, bstring target, bstring *out_pattern)
{
    Route *found = NULL;

    found = RouteMap_simple_prefix_match(host->routes, target);

    if(found) {
        debug("Found backend at %s", bdata(found->pattern));
        assert(found->data && "Invalid value for stored route.");
        if(out_pattern) *out_pattern = found->pattern;
        return found->data;
    } else {
        if(out_pattern) *out_pattern = NULL;
        return NULL;
    }
}

