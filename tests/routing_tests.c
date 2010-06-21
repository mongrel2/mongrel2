#include "minunit.h"
#include <routing.h>
#include <string.h>
#include <dbg.h>

FILE *LOG_FILE = NULL;


int check_routing(list_t *found, Route *route, int expected_count, const char *route_data)
{
    check(list_count(found) == expected_count, "Didn't find right number of routes.");
    route = lnode_get(list_first(found));
    debug("Route returned: %s", route->data);
    check(route->data == route_data, "Wrong data returned.");
    return 1;

error:
    return 0;
}


char *test_routing_match() 
{
    RouteMap *routes = RouteMap_create();
    mu_assert(routes != NULL, "Failed to make the route map.");
    char *route_data1 = "route1";
    char *route_data2 = "route2";
    char *route_data3 = "route3";
    Route *route = NULL;

    RouteMap_insert(routes, "/users/([0-9]+)", strlen("/users/([0-9]+)"), route_data1);
    RouteMap_insert(routes, "/users", strlen("/users"), route_data2);

    // TODO: seems you can't have more than one match for a path, need to find out why
    // if this is changed to be /users/([a-z]-)$ it causes route1 to fail.
    RouteMap_insert(routes, "/cars/([a-z]-)$", strlen("/cars/([a-z]-)$"), route_data3);

    list_t *found = RouteMap_match(routes, "/users/1234", strlen("/users/1234"));
    mu_assert(check_routing(found, route, 1, route_data1), "Pattern match route wrong.");

    // must make sure that match is partial unless $ explicitly
    found = RouteMap_match(routes, "/users/1234/testing", strlen("/users/1234/testing"));
    mu_assert(check_routing(found, route, 1, route_data1), "Past end route wrong.");


    found = RouteMap_match(routes, "/users", strlen("/users"));
    mu_assert(check_routing(found, route, 1, route_data2), "No pattern route wrong.");

    found = RouteMap_match(routes, "/cars/cadillac", strlen("/cars/cadillac"));
    mu_assert(check_routing(found, route, 1, route_data3), "Wrong $ terminated route.");

    return NULL;
}


char * all_tests() {
    mu_suite_start();

    mu_run_test(test_routing_match);

    return NULL;
}

RUN_TESTS(all_tests);


