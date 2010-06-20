/* file minunit_example.c */
 
#include "minunit.h"
#include <host.h>

FILE *LOG_FILE = NULL;

char * test_create_destroy() {
    Host *host = Host_create("zedshaw.com");

    mu_assert(host != NULL, "Failed to make host.");

    Host_destroy(host);

    return NULL;
}

char *all_tests() {
    mu_suite_start();

    mu_run_test(test_create_destroy);

    return NULL;
}

RUN_TESTS(all_tests);

