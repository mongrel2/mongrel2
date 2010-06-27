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

Route *RouteMap_insert_base(RouteMap *routes, 
        const char *prefix, size_t prefix_len,
        const char *pattern, size_t pattern_len)
{
    Route *route = malloc(sizeof(Route));
    check(route, "Out of memory.");

    route->length = pattern_len;
    route->pattern = strdup(pattern);
    check(route->pattern, "Error copying pattern.");
    route->pattern[pattern_len] = '\0';  // make sure it's capped

    routes->routes = tst_insert(routes->routes, prefix, prefix_len, route);

    check(routes->routes, "Failed to insert into TST.");

    return route;

error:
    return NULL;
}

int RouteMap_insert(RouteMap *routes, const char *pattern, size_t len, void *data)
{
    Route *route = NULL;
    // WARNING: dangerous code potentially
    char *first_paren = strchr(pattern, '(');
    size_t pref_len = first_paren ? first_paren - pattern : len;

    route = RouteMap_insert_base(routes, pattern, pref_len, pattern, len);
    check(route, "Failed to insert route: %.*s", (int)len, pattern);

    route->data = data;

    return 0;

error:
    return -1;
}

int RouteMap_insert_reversed(RouteMap *routes, const char *pattern, size_t len, void *data)
{
    Route *route = NULL;
    char reversed_prefix[MAX_HOST_NAME];
    char *last_paren = strrchr(pattern, ')');
    size_t prefix_len = last_paren ? len - (last_paren - pattern + 1) : len;
    int pi, ri;

    debug("prefix_len: %d", (int)prefix_len);

    for(pi = len-1, ri = 0; ri < prefix_len; ri++, pi--) {
        reversed_prefix[ri] = pattern[pi];
    }
    reversed_prefix[prefix_len] = '\0';

    debug("prefix: %.*s, pattern: %.*s", (int)prefix_len, reversed_prefix, (int)len, pattern);

    route = RouteMap_insert_base(routes, reversed_prefix, prefix_len, pattern, len);
    check(route, "Failed to add host.");
    route->data = data;

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


