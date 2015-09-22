#include "minunit.h"
#include <routing.h>
#include <string.h>
#include <dbg.h>


int check_routing(list_t *found, Route *route, int expected_count, const char *route_data)
{
    if(list_first(found)) {
        route = lnode_get(list_first(found));
    }

    check((int)list_count(found) == expected_count, "Didn't find right number of routes: got %d, expected %d.", 
            (int)list_count(found), expected_count);

    if(route_data == NULL) {
        check(route == NULL, "Should have no routes found.");
    } else {
        check(route->data == route_data, "Wrong data returned.");
    }
    return 1;

error:
    if(route) debug("Route returned: %s", (const char *)route->data);
    return 0;
}


int check_simple_prefix(RouteMap *routes, const char* path, const char *expected)
{
    bstring path_str = bfromcstr(path);
    Route *found = RouteMap_simple_prefix_match(routes, path_str);
    debug("Testing simple prefix: %s to match %s and found %s",
            path, expected, found == NULL ? "NULL" : (const char *)found->data);

    if(expected != NULL) {
        check(found != NULL, "Should find a match.");
        check(found->data == expected, "Didn't match.");
    } else {
        check(found == NULL, "Should not find a match.");
    }

    bdestroy(path_str);
    return 1;
error: 
    bdestroy(path_str);
    return 0;
}

char *test_simple_prefix_matching() 
{
    RouteMap *routes = RouteMap_create(NULL);
    mu_assert(routes != NULL, "Failed to make the route map.");
    char *route_data0 = "route0";
    char *route_data1 = "route1";
    char *route_data2 = "route2";
    char *route_data3 = "route3";
    char *route_data4 = "route4";
    bstring route0 = bfromcstr("/");
    bstring route1 = bfromcstr("/users/([0-9]+)");
    bstring route2 = bfromcstr("/users");
    bstring route3 = bfromcstr("/users/people/([0-9]+)$");
    bstring route4 = bfromcstr("/cars-fast/([a-z]-)$");

    RouteMap_insert(routes, route0, route_data0);
    RouteMap_insert(routes, route1, route_data1);
    RouteMap_insert(routes, route2, route_data2);
    RouteMap_insert(routes, route3, route_data3);
    RouteMap_insert(routes, route4, route_data4);

    mu_assert(check_simple_prefix(routes, "/users/1234/testing", route_data1), "Failed.");
    mu_assert(check_simple_prefix(routes, "/users", route_data2), "Failed.");  
    mu_assert(check_simple_prefix(routes, "/users/people/1234", route_data3), "Failed.");  
    mu_assert(check_simple_prefix(routes, "/cars-fast/cadillac", route_data4), "Failed.");  
    mu_assert(check_simple_prefix(routes, "/users/1234", route_data1), "Failed.");  
    mu_assert(check_simple_prefix(routes, "/", route_data0), "Failed.");

    // this is expected, because /users isn't explicitly patterned to be exact, it 
    // only prefix matches.
    mu_assert(check_simple_prefix(routes, "/usersBLAAHAHAH", route_data2), "Failed.");

    mu_assert(check_simple_prefix(routes, "/us", route_data2), "Failed.");

    RouteMap_destroy(routes);

    return NULL;
}
char *test_simple_prefix_pattern_matching() 
{
    RouteMap *routes = RouteMap_create(NULL);
    mu_assert(routes != NULL, "Failed to make the route map.");
    char *route_data0 = "route0";
    char *route_data1 = "route1";
    char *route_data2 = "route2";
    bstring route0 = bfromcstr("/");
    bstring route1 = bfromcstr("/img/(.*).jpg$");
    bstring route2 = bfromcstr("/img/(.*).png$");

    RouteMap_insert(routes, route0, route_data0);
    RouteMap_insert(routes, route1, route_data1);
    RouteMap_insert(routes, route2, route_data2);

    mu_assert(check_simple_prefix(routes, "/img/foo.jpg", route_data1), "Failed 1.");
    mu_assert(check_simple_prefix(routes, "/img/foo.png", route_data2), "Failed 2.");  
    mu_assert(check_simple_prefix(routes, "/img/foo.bmp", NULL), "Failed 3.");  

    RouteMap_destroy(routes);

    return NULL;
}
char *test_routing_match() 
{
    RouteMap *routes = RouteMap_create(NULL);
    mu_assert(routes != NULL, "Failed to make the route map.");
    char *route_data0 = "route0";
    char *route_data1 = "route1";
    char *route_data2 = "route2";
    char *route_data3 = "route3";
    char *route_data4 = "route4";
    bstring route0 = bfromcstr("/");
    bstring route1 = bfromcstr("/users/([0-9]+)");
    bstring route2 = bfromcstr("/users");
    bstring route3 = bfromcstr("/users/people/([0-9]+)$");
    bstring route4 = bfromcstr("/cars-fast/([a-z]-)$");

    Route *route = NULL;

    RouteMap_insert(routes, route0, route_data0);
    RouteMap_insert(routes, route1, route_data1);
    RouteMap_insert(routes, route2, route_data2);
    RouteMap_insert(routes, route3, route_data3);
    RouteMap_insert(routes, route4, route_data4);

    bstring path1 = bfromcstr("/users/1234/testing");
    bstring path2 = bfromcstr("/users");
    bstring path3 = bfromcstr("/users/people/1234"); 
    bstring path4 = bfromcstr("/cars-fast/cadillac");
    bstring path5 = bfromcstr("/users/1234");
    bstring path6 = bfromcstr("/");
    bstring path7 = bfromcstr("/users/people/1234/notgonnawork");

    list_t *found = RouteMap_match(routes, path5);
    mu_assert(check_routing(found, route, 1, route_data1), "Pattern match route wrong.");
    list_destroy_nodes(found);
    list_destroy(found);

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

    found = RouteMap_match(routes, path5);
    mu_assert(check_routing(found, route, 1, route_data1), "Wrong route for /users/1234");
    list_destroy_nodes(found);
    list_destroy(found);

    found = RouteMap_match(routes, path6);
    mu_assert(check_routing(found, route, 2, route_data0), "Should get root / route.");
    list_destroy_nodes(found);
    list_destroy(found);

    found = RouteMap_match(routes, path7);
    mu_assert(check_routing(found, route, 0, NULL), "Should not match past end");
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


char *test_routing_match_reversed() 
{
    RouteMap *routes = RouteMap_create(NULL);
    mu_assert(routes != NULL, "Failed to make the route map.");
    char *route_data1 = "route1";
    char *route_data2 = "route2";
    char *route_data3 = "route3";
    bstring route1 = bfromcstr("(X+)foo");
    bstring route2 = bfromcstr("moo");
    bstring route3 = bfromcstr("fio");

    Route *route = NULL;

    RouteMap_insert_reversed(routes, route1, route_data1);
    RouteMap_insert_reversed(routes, route2, route_data2);
    RouteMap_insert_reversed(routes, route3, route_data3);

    bstring path1 = bfromcstr("Xfoo");
    bstring path2 = bfromcstr("moo");
    bstring path3 = bfromcstr("fio");
    bstring path4 = bfromcstr("XXXXXXXXXfoo");
    bstring path5 = bfromcstr("YYYfoo");
    bstring path6 = bfromcstr("XXXoo");
    bstring path7 = bfromcstr("XXX.stuff.foo");

    route = RouteMap_match_suffix(routes, path1);
    mu_assert(route != NULL, "Pattern match failed.");
    mu_assert(route->data == route_data1, "Pattern matched wrong route.");

    route = RouteMap_match_suffix(routes, path2);
    mu_assert(route != NULL, "Pattern match failed.");
    mu_assert(route->data == route_data2, "Pattern matched wrong route.");

    route = RouteMap_match_suffix(routes, path3);
    mu_assert(route != NULL, "Pattern match failed.");
    mu_assert(route->data == route_data3, "Pattern matched wrong route.");

    route = RouteMap_match_suffix(routes, path4);
    mu_assert(route != NULL, "Pattern match failed.");
    mu_assert(route->data == route_data1, "Pattern matched wrong route.");

    route = RouteMap_match_suffix(routes, path5);
    mu_assert(route == NULL, "Pattern match failed.");

    route = RouteMap_match_suffix(routes, path6);
    mu_assert(route == NULL, "Pattern match failed.");

    route = RouteMap_match_suffix(routes, path7);
    mu_assert(route == NULL, "Pattern match failed.");

    bdestroy(path1);
    bdestroy(path2);
    bdestroy(path3);
    bdestroy(path4);
    bdestroy(path5);
    bdestroy(path6);

    RouteMap_destroy(routes);

    return NULL;
}


char * all_tests() {
    mu_suite_start();

    mu_run_test(test_routing_match);
    mu_run_test(test_simple_prefix_matching);
    mu_run_test(test_routing_match_reversed);
    mu_run_test(test_simple_prefix_pattern_matching);

    return NULL;
}

RUN_TESTS(all_tests);


