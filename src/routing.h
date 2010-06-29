#ifndef _routing_h
#define _routing_h

#include <adt/tst.h>
#include <stdlib.h>
#include <adt/list.h>
#include <bstring.h>

typedef struct RouteMap {
    tst_t *routes;
} RouteMap;

typedef struct Route {
    bstring pattern;
    void *data;
} Route;

RouteMap *RouteMap_create();

int RouteMap_insert(RouteMap *routes, bstring pattern, void *data);

list_t *RouteMap_match(RouteMap *routes, bstring path);

int RouteMap_insert_reversed(RouteMap *routes, bstring pattern, void *data);


#endif
