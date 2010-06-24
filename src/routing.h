#ifndef _routing_h
#define _routing_h

#include <adt/tst.h>
#include <stdlib.h>
#include <adt/list.h>

typedef struct RouteMap {
    tst_t *routes;
} RouteMap;

typedef struct Route {
    char *pattern;
    size_t length;
    void *data;
} Route;

RouteMap *RouteMap_create();

int RouteMap_insert(RouteMap *routes, const char *pattern, size_t len, void *data);

list_t *RouteMap_match(RouteMap *routes, const char *path, size_t len);

int RouteMap_insert_reversed(RouteMap *routes, const char *pattern, size_t len, void *data);


#endif
