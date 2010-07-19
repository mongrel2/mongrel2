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



RouteMap *RouteMap_create()
{
    RouteMap *map = h_calloc(sizeof(RouteMap), 1);
    return map;
}

void RouteMap_destroy(RouteMap *map)
{
    // TODO: add a callback for destroying the contents, or h_malloc required
    if(map) {
        tst_destroy(map->routes);
        h_free(map);
    }
}

Route *RouteMap_insert_base(RouteMap *map, bstring prefix, bstring pattern)
{
    Route *route = h_malloc(sizeof(Route));
    check_mem(route);

    route->pattern = pattern;
    check(route->pattern, "Pattern is required.");

    debug("ADDING prefix: %s, pattern: %s", bdata(prefix), bdata(pattern));

    map->routes = tst_insert(map->routes, bdata(prefix), blength(prefix), route);

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
    route->data = data;

    bdestroy(prefix);
    return 0;

error:
    bdestroy(prefix);
    return -1;
}

int RouteMap_insert_reversed(RouteMap *map, bstring pattern, void *data)
{
    Route *route = NULL;
    bstring reversed_prefix = NULL;

    int last_paren = bstrrchr(pattern, ')');
    if(last_paren >= 0) {
        reversed_prefix = bTail(pattern, blength(pattern) - last_paren - 1);
    } else {
        reversed_prefix = bstrcpy(pattern);
    }

    check_mem(reversed_prefix);
    bReverse(reversed_prefix);

    // we own reversed_prefix, they own pattern
    route = RouteMap_insert_base(map, reversed_prefix, pattern);
    check(route, "Failed to add host.");

    route->data = data;

    bdestroy(reversed_prefix);
    return 0;
  
error:
    bdestroy(reversed_prefix);
    return -1;
}


int RouteMap_collect_match(void *value, const char *key, size_t len)
{
    assert(value && "NULL value from TST.");
    Route *route = (Route *)value;

    return pattern_match(key, len, bdata(route->pattern)) != NULL;
}

list_t *RouteMap_match(RouteMap *map, bstring path)
{
    return tst_collect(map->routes, bdata(path),
            blength(path), RouteMap_collect_match);
}


list_t *RouteMap_match_suffix(RouteMap *map, bstring target)
{
    // TODO: create a suffix collect so we don't have to do this
    bstring reversed = bstrcpy(target);
    bReverse(reversed);

    list_t *results = tst_collect(map->routes, bdata(reversed),
                        blength(reversed), NULL);

    bdestroy(reversed);

    return results;
}


Route *RouteMap_simple_prefix_match(RouteMap *map, bstring target)
{ 
    Route *route = tst_search_prefix(map->routes, bdata(target), blength(target));

    if(route) {
        debug("Found simple prefix: %s", bdata(route->pattern));
        return bstring_match(target, route->pattern) ? route : NULL;
    } else {
        return NULL;
    }
}
