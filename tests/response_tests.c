#include "minunit.h"
#include <response.h>

FILE *LOG_FILE = NULL;

char *test_Response_send_status() 
{
    int rc = Response_send_status(2, &HTTP_304);
    mu_assert(rc == -1, "This SHOULD fail.");

    return NULL;
}

char *test_Response_send_socket_policy()
{
    int rc = Response_send_socket_policy(2);
    mu_assert(rc == -1, "This SHOULD fail.");

    return NULL;
}


char * all_tests() {
    mu_suite_start();

    mu_run_test(test_Response_send_status);
    mu_run_test(test_Response_send_socket_policy);

    return NULL;
}

RUN_TESTS(all_tests);

