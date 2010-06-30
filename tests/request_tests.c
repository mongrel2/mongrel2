#include "minunit.h"
#include <request.h>

FILE *LOG_FILE = NULL;

char *test_Request_create() 
{
 
    Request *req = Request_create();
    mu_assert(req != NULL, "Failed to create parser for request.");

    Request_destroy(req);

    return NULL;
}


char * all_tests() {
    mu_suite_start();

    mu_run_test(test_Request_create);

    return NULL;
}

RUN_TESTS(all_tests);

