#include <routing.h>
#include <dbg.h>
#include <string.h>
#include <assert.h>
#include <pattern.h>



RouteMap *RouteMap_create()
{
    RouteMap *map = calloc(sizeof(RouteMap), 1);
    return map;
}


int RouteMap_insert(RouteMap *routes, const char *pattern, size_t len, void *data)
{
    Route *route = malloc(sizeof(RouteMap));
    check(route, "Out of memory.");

    route->pattern = strdup(pattern);
    check(route->pattern, "Error copying pattern.");
    route->pattern[len] = '\0';  // make sure it's capped

    route->length = len;
    route->data = data;

    // WARNING: dangerous code potentially
    char *first_paren = strchr(route->pattern, '(');

    if(first_paren) {
        debug("Route added with pattern: %s until %.*s", route->pattern, 
                first_paren - route->pattern, route->pattern);
        routes->routes = tst_insert(routes->routes, route->pattern,
                first_paren - route->pattern, route);
    } else {
        routes->routes = tst_insert(routes->routes, route->pattern, len, route);
    }

    check(routes->routes, "Failed to insert into TST.");

    return 0;

error:
    return -1;
}

int RouteMap_collect_match(void *value, const char *key, size_t len)
{
    assert(value && "NULL value from TST.");
    Route *route = (Route *)value;

    return pattern_match(key, len, route->pattern) != NULL;
}

list_t *RouteMap_match(RouteMap *routes, const char *path, size_t len)
{
    return tst_collect(routes->routes, path, len, RouteMap_collect_match);
}


