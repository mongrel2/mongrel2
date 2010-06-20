#include "minunit.h"
#include <register.h>

FILE *LOG_FILE = NULL;

char *test_Register_init() 
{
    Register_init();

    return NULL;
}

char *test_Register_connect_disconnect()
{
    Register_connect(12);
    mu_assert(Register_exists(12) == 1, "Didn't register.");

    Register_disconnect(12);
    mu_assert(Register_exists(12) == 0, "Didn't disconnect.");

    return NULL;
}

char *test_Register_ping()
{
    Register_connect(12232);
    mu_assert(Register_exists(12232) == 1, "Didn't register.");

    mu_assert(Register_ping(12232), "Ping didn't work.");

    Register_disconnect(12232);
    mu_assert(Register_exists(12) == 0, "Didn't disconnect.");

    return NULL;
}


char * all_tests() {
    mu_suite_start();

    mu_run_test(test_Register_init);
    mu_run_test(test_Register_connect_disconnect);
    mu_run_test(test_Register_ping);

    return NULL;
}

RUN_TESTS(all_tests);

