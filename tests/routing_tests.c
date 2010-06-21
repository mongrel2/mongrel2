#include "minunit.h"
#include <routing.h>
#include <string.h>

FILE *LOG_FILE = NULL;

char *test_routing_match() 
{
    RouteMap *routes = RouteMap_create();
    mu_assert(routes != NULL, "Failed to make the route map.");

    RouteMap_insert(routes, "/users/([0-9]+)", strlen("/users/([0-9]+)"), "route1");
    RouteMap_insert(routes, "/users", strlen("/users"), "route2");

    // TODO: seems you can't have more than one match for a path, need to find out why
    // if this is changed to be /users/([a-z]-)$ it causes route1 to fail.
    RouteMap_insert(routes, "/cars/([a-z]-)$", strlen("/cars/([a-z]-)$"), "route3");

    list_t *found = RouteMap_match(routes, "/users/1234", strlen("/users/1234"));
    mu_assert(list_count(found) == 1, "Didn't find the route with pattern.");

    // must make sure that match is partial unless $ explicitly
    found = RouteMap_match(routes, "/users/1234/testing", strlen("/users/1234/testing"));
    mu_assert(list_count(found) == 1, "Didn't find the route with pattern past end.");

    found = RouteMap_match(routes, "/users", strlen("/users"));
    mu_assert(list_count(found) == 1, "Didn't find the route without pattern.");

    found = RouteMap_match(routes, "/cars/cadillac", strlen("/cars/cadillac"));
    mu_assert(list_count(found) == 1, "Didn't find the route with $ anchor.");

    return NULL;
}


char * all_tests() {
    mu_suite_start();

    mu_run_test(test_routing_match);

    return NULL;
}

RUN_TESTS(all_tests);


