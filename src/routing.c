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

#include <routing.h>
#include <dbg.h>
#include <string.h>
#include <assert.h>
#include <pattern.h>
#include <host.h>
#include <mem/halloc.h>
#include <bstring.h>



RouteMap *RouteMap_create(routemap_destroy_cb destroy)
{
    RouteMap *map = h_calloc(sizeof(RouteMap), 1);
    check_mem(map);

    map->destroy = destroy;

    return map;

error:
    return NULL;
}

void RouteMap_cleanup(void *value, void *data)
{
    Route *route = (Route *)value;
    RouteMap *map = (RouteMap *)data;

    if(map->destroy) {
        map->destroy(route, map);
    }

    bdestroy(route->pattern);
    bdestroy(route->prefix);
}

void RouteMap_destroy(RouteMap *map)
{
    if(map) {
        tst_traverse(map->routes, RouteMap_cleanup, map);
        tst_destroy(map->routes);
        h_free(map);
    }
}

Route *RouteMap_insert_base(RouteMap *map, bstring prefix, bstring pattern)
{
    Route *route = h_malloc(sizeof(Route));
    Route *route2;
    check_mem(route);

    route->pattern = pattern;
    check(route->pattern, "Pattern is required.");

    route->prefix = prefix;
    check(route->prefix, "Prefix is required.");
    route->next = NULL;

    debug("ADDING prefix: %s, pattern: %s", bdata(prefix), bdata(pattern));
    route2= tst_search(map->routes, bdata(prefix), blength(prefix));
    
    if (route2 != NULL) {
        route->next=route2->next;
        route2->next=route;
    }
    else {
        map->routes = tst_insert(map->routes, bdata(prefix), blength(prefix), route);
    }

    hattach(route, map);

    check(map->routes, "Failed to insert into TST.");

    return route;

error:
    return NULL;
}

int RouteMap_insert(RouteMap *map, bstring pattern, void *data)
{
    Route *route = NULL;
    int first_paren = bstrchr(pattern, '(');
    bstring prefix = NULL;

    if(first_paren >= 0) {
        prefix = bHead(pattern, first_paren);
    } else {
        prefix = bstrcpy(pattern);
    }

    check_mem(prefix);

    // pattern is owned by RouteMap, prefix is owned by us
    route = RouteMap_insert_base(map, prefix, pattern);
    check(route, "Failed to insert route: %s", bdata(pattern));

    // TODO: figure out whether we can hattach the data too
    route->has_pattern = first_paren >= 0;
    route->first_paren = first_paren;
    route->data = data;

    return 0;

error:
    return -1;
}

int RouteMap_insert_reversed(RouteMap *map, bstring pattern, void *data)
{
    Route *route = NULL;
    bstring reversed_prefix = NULL;

    int last_paren = bstrrchr(pattern, ')');

    if(last_paren >= 0) {
        // reversed patterns put the pattern first, so it needs to be checked as a complete whole
        reversed_prefix = bTail(pattern, blength(pattern) - last_paren - 1);
        btrunc(pattern, last_paren + 1);
        bconchar(pattern, '$');
    } else {
        reversed_prefix = bstrcpy(pattern);
    }

    check_mem(reversed_prefix);
    bReverse(reversed_prefix);

    route = RouteMap_insert_base(map, reversed_prefix, pattern);
    check(route, "Failed to add host.");

    route->has_pattern = last_paren >= 0;
    route->prefix = reversed_prefix;
    route->data = data;

    return 0;
  
error:
    return -1;
}


int RouteMap_collect_match(void *value, const char *key, size_t len)
{
    assert(value && "NULL value from TST.");
    Route *route = (Route *)value;

    if(route->has_pattern) {
        if(route->first_paren >= 0 && (size_t)route->first_paren < len) {
            // it's not possible to match if the pattern paren is after the value we're testing
            return pattern_match(key + route->first_paren,
                    len - route->first_paren,
                    bdataofs(route->pattern, route->first_paren)) != NULL;
        } else {
            return 0;
        }
    } else {
        return 1;
    }
}

list_t *RouteMap_match(RouteMap *map, bstring path)
{
    return tst_collect(map->routes, bdata(path),
            blength(path), RouteMap_collect_match);
}

static inline Route *match_route_pattern(bstring target, Route *route, int suffix)
{
    const char *source = suffix ? bdata(target) : bdataofs(target, blength(route->prefix));
    int source_length = blength(target) - blength(route->prefix);
    const char *pattern = suffix ? bdata(route->pattern) : bdataofs(route->pattern, route->first_paren);

    if(source_length >= 0 && source != NULL) {
        return pattern_match(source, source_length, pattern) ? route : NULL;
    } else {
        return NULL;
    }
}

Route *RouteMap_match_suffix(RouteMap *map, bstring target)
{
    Route *route = tst_search_suffix(map->routes, bdata(target), blength(target));

    if(route) {
        debug("Found simple suffix: %s for target: %s", bdata(route->pattern), bdata(target));

        if(route->has_pattern) {
            return match_route_pattern(target, route, 1);
        } else {
            return route;
        }
    } else {
        return NULL;
    }
}


Route *RouteMap_simple_prefix_match(RouteMap *map, bstring target)
{
    debug("Searching for route: %s in map: %p", bdata(target), map);
    Route *route = tst_search_prefix(map->routes, bdata(target), blength(target));

    if(route) {
        debug("Found simple prefix: %s", bdata(route->pattern));

        if(route->has_pattern) {
            while(route != NULL)
            {
                Route *matched = match_route_pattern(target, route, 0);
                if(matched != NULL) {
                    return matched;
                }
                route=route->next;
            }
        } else {
            return route;
        }
    }

    return NULL;
}
