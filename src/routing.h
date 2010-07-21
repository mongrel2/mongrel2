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

#ifndef _routing_h
#define _routing_h

#include <adt/tst.h>
#include <stdlib.h>
#include <adt/list.h>
#include <bstring.h>

typedef struct Route {
    bstring pattern;
    void *data;
} Route;

struct RouteMap;

typedef void (*routemap_destroy_cb)(Route *route, struct RouteMap *map);

typedef struct RouteMap {
    tst_t *routes;
    routemap_destroy_cb destroy;
} RouteMap;

RouteMap *RouteMap_create(routemap_destroy_cb destroy);

void RouteMap_destroy(RouteMap *map);

int RouteMap_insert(RouteMap *routes, bstring pattern, void *data);

list_t *RouteMap_match(RouteMap *routes, bstring path);

int RouteMap_insert_reversed(RouteMap *routes, bstring pattern, void *data);

list_t *RouteMap_match_suffix(RouteMap *map, bstring target);

Route *RouteMap_simple_prefix_match(RouteMap *map, bstring target);

#endif
