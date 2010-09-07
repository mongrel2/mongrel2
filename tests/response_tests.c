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
    Connection conn = {0};
    conn.fd = 2;
    conn.send = my_send;

    int rc = Response_send_status(&conn, &HTTP_304);
    mu_assert(rc == -1, "This SHOULD fail.");

    return NULL;
}

char *test_Response_send_socket_policy()
{
    Connection conn = {0};
    conn.fd = 2;
    conn.send = my_send;

    int rc = Response_send_socket_policy(&conn);
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

