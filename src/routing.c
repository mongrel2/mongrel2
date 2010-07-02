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
    check(route, "Out of memory.");

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

    check(prefix, "Couldn't create prefix: %s", bdata(pattern));

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

    check(reversed_prefix, "Failed to create prefix to reverse.");
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

    debug("route matching: %.*s with pattern: %s", len, key, bdata(route->pattern));

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

