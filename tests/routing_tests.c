#include "minunit.h"
#include <routing.h>
#include <string.h>
#include <dbg.h>

FILE *LOG_FILE = NULL;


int check_routing(list_t *found, Route *route, int expected_count, const char *route_data)
{
    check(list_count(found) == expected_count, "Didn't find right number of routes: got %d, expected %d.", 
            (int)list_count(found), expected_count);
    route = lnode_get(list_first(found));
    debug("Route returned: %s", (const char *)route->data);
    check(route->data == route_data, "Wrong data returned.");
    return 1;

error:
    return 0;
}


char *test_routing_match() 
{
    RouteMap *routes = RouteMap_create(NULL);
    mu_assert(routes != NULL, "Failed to make the route map.");
    char *route_data1 = "route1";
    char *route_data2 = "route2";
    char *route_data3 = "route3";
    char *route_data4 = "route4";
    bstring route1 = bfromcstr("/users/([0-9]+)");
    bstring route2 = bfromcstr("/users");
    bstring route3 = bfromcstr("/users/people/([0-9]+))$");
    bstring route4 = bfromcstr("/cars-fast/([a-z]-)$");

    Route *route = NULL;

    RouteMap_insert(routes, route1, route_data1);
    RouteMap_insert(routes, route2, route_data2);
    RouteMap_insert(routes, route3, route_data4);
    RouteMap_insert(routes, route4, route_data3);

    bstring path1 = bfromcstr("/users/1234/testing");
    bstring path2 = bfromcstr("/users");
    bstring path3 = bfromcstr("/cars-fast/cadillac");
    bstring path4 = bfromcstr("/users/people/1234"); 
    bstring path5 = bfromcstr("/users/1234");

    list_t *found = RouteMap_match(routes, path5);
    mu_assert(check_routing(found, route, 1, route_data1), "Pattern match route wrong.");
    list_destroy_nodes(found);
    list_destroy(found);

    // must make sure that match is partial unless $ explicitly
    found = RouteMap_match(routes, path1);
    mu_assert(check_routing(found, route, 1, route_data1), "Past end route wrong.");
    list_destroy_nodes(found);
    list_destroy(found);

    found = RouteMap_match(routes, path2);
    mu_assert(check_routing(found, route, 1, route_data2), "No pattern route wrong.");
    list_destroy_nodes(found);
    list_destroy(found);

    found = RouteMap_match(routes, path3);
    mu_assert(check_routing(found, route, 1, route_data3), "Wrong $ terminated route.");
    list_destroy_nodes(found);
    list_destroy(found);

    found = RouteMap_match(routes, path4);
    mu_assert(check_routing(found, route, 1, route_data4), "Wrong longer route match.");
    list_destroy_nodes(found);
    list_destroy(found);

    bdestroy(path1);
    bdestroy(path2);
    bdestroy(path3);
    bdestroy(path4);
    bdestroy(path5);

    RouteMap_destroy(routes);

    return NULL;
}


char * all_tests() {
    mu_suite_start();

    mu_run_test(test_routing_match);

    return NULL;
}

RUN_TESTS(all_tests);


