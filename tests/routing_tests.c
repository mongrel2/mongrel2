#include "minunit.h"
#include <routing.h>
#include <string.h>

FILE *LOG_FILE = NULL;

char *test_routing_match() 
{
    RouteMap *routes = RouteMap_create();
    mu_assert(routes != NULL, "Failed to make the route map.");

    // TODO:  must make sure that match is partial unless $ explicitly
    RouteMap_insert(routes, "/users/([0-9]*)", strlen("/users/([0-9]*)"), NULL);

    return NULL;
}


char * all_tests() {
    mu_suite_start();

    mu_run_test(test_routing_match);

    return NULL;
}

RUN_TESTS(all_tests);


