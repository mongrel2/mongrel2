#include "minunit.h"
#include <response.h>
#include <connection.h>

FILE *LOG_FILE = NULL;

static ssize_t my_send(Connection *conn, char *buff, int len)
{
    return -1;
}

char *test_Response_send_status() 
{

    return "REWRITE NEEDED";
}

char *test_Response_send_socket_policy()
{

    return "REWRITE NEEDED";
}


char * all_tests() {
    mu_suite_start();

    mu_run_test(test_Response_send_status);
    mu_run_test(test_Response_send_socket_policy);

    return NULL;
}

RUN_TESTS(all_tests);

