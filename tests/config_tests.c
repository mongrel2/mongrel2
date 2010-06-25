#include "minunit.h"
#include <config/config.h>

FILE *LOG_FILE = NULL;

char *test_Config_load() 
{
    list_t *servers = Config_load("tests/config.sqlite");

    mu_assert(servers != NULL, "Should get a server list.");
    mu_assert(list_count(servers) == 1, "Failed to load the server.");

    return NULL;
}


char * all_tests() {
    mu_suite_start();

    Server_init();

    mu_run_test(test_Config_load);

    return NULL;
}

RUN_TESTS(all_tests);

