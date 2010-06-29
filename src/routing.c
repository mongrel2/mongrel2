#include <routing.h>
#include <dbg.h>
#include <string.h>
#include <assert.h>
#include <pattern.h>
#include <host.h>



RouteMap *RouteMap_create()
{
    RouteMap *map = calloc(sizeof(RouteMap), 1);
    return map;
}

Route *RouteMap_insert_base(RouteMap *routes, bstring prefix, bstring pattern)
{
    Route *route = malloc(sizeof(Route));
    check(route, "Out of memory.");

    route->pattern = pattern;
    check(route->pattern, "Pattern is required.");

    debug("ADDING prefix: %s, pattern: %s", bdata(prefix), bdata(pattern));

    routes->routes = tst_insert(routes->routes, bdata(prefix), blength(prefix), route);
    check(routes->routes, "Failed to insert into TST.");

    return route;

error:
    return NULL;
}

int RouteMap_insert(RouteMap *routes, bstring pattern, void *data)
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
    route = RouteMap_insert_base(routes, prefix, pattern);

    check(route, "Failed to insert route: %s", bdata(pattern));

    route->data = data;

    bdestroy(prefix);
    return 0;

error:
    bdestroy(prefix);
    return -1;
}

int RouteMap_insert_reversed(RouteMap *routes, bstring pattern, void *data)
{
    Route *route = NULL;
    bstring reversed_prefix = NULL;

    int last_paren = bstrrchr(pattern, ')');
    if(last_paren >= 0) {
        reversed_prefix = bTail(pattern, last_paren);
    } else {
        reversed_prefix = bstrcpy(pattern);
    }

    check(reversed_prefix, "Failed to create prefix to reverse.");
    bReverse(reversed_prefix);

    // we own reversed_prefix, they own pattern
    route = RouteMap_insert_base(routes, reversed_prefix, pattern);
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

list_t *RouteMap_match(RouteMap *routes, bstring path)
{
    return tst_collect(routes->routes, bdata(path),
            blength(path), RouteMap_collect_match);
}


