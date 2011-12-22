#include "minunit.h"
#include <connection.h>
#include <register.h>

int V_TEST_CONN_1 = 1;
int V_TEST_CONN_2 = 2;
Connection *TEST_CONN_1 = NULL;
Connection *TEST_CONN_2 = NULL;

char *test_Register_init() 
{
    Register_init();

    TEST_CONN_1 = calloc(sizeof(Connection), 1);
    TEST_CONN_2 = calloc(sizeof(Connection), 1);

    return NULL;
}

char *test_Register_connect_disconnect()
{
    int rc = Register_connect(12, TEST_CONN_1);
    mu_assert(Register_fd_exists(12) == TEST_CONN_1, "Didn't register.");
    mu_assert(rc == 0, "Got a negative for the ident.");

    uint32_t id = Register_id_for_fd(12);
    mu_assert(id != UINT32_MAX, "Invalid id when trying to get it.");

    int fd = Register_fd_for_id(id);
    mu_assert(fd == 12, "Should get 12 for the fd.");

    rc = Register_disconnect(12);
    mu_assert(Register_fd_exists(12) == 0, "Didn't disconnect.");
    mu_assert(rc != -1, "Wrong id on disconnect.");

    // do it again to make sure we log invalid unregisters
    rc = Register_disconnect(12);
    mu_assert(rc == -1, "Should get an error.");
    return NULL;
}

char *test_Register_ping()
{
    int rc = Register_connect(12232, TEST_CONN_1);
    mu_assert(rc != -1, "Failed to connect 12232.");
    mu_assert(Register_fd_exists(12232) == TEST_CONN_1, "Didn't register.");

    mu_assert(Register_ping(12232), "Ping didn't work.");

    // attempt a double registration which should NOT work
    rc = Register_connect(12232, TEST_CONN_2);
    mu_assert(rc != -1, "Second register should get new id.");
    mu_assert(Register_fd_exists(12232) == TEST_CONN_2, "Should have different connection");

    rc = Register_disconnect(12232);
    mu_assert(rc != -1, "Didn't disconnect after some pings.");
    mu_assert(task_was_signaled(), "Should be signaled for attempting a double reg.");

    task_clear_signal();

    mu_assert(Register_fd_exists(12232) == 0, "Disconnect didn't work.");

    mu_assert(Register_ping(12) == -1, "Should get error on ping of disconnect.");

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

