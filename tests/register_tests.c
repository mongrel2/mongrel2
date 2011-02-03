#include "minunit.h"
#include <register.h>

int V_TEST_CONN_1 = 1;
int V_TEST_CONN_2 = 2;
void *TEST_CONN_1 = &V_TEST_CONN_1;
void *TEST_CONN_2 = &V_TEST_CONN_2;

FILE *LOG_FILE = NULL;

char *test_Register_init() 
{
    Register_init();

    return NULL;
}

char *test_Register_connect_disconnect()
{
    int id = Register_connect(12, TEST_CONN_1);
    mu_assert(Register_fd_exists(12) == TEST_CONN_1, "Didn't register.");
    mu_assert(id >= 0, "Got a negative for the ident.");

    int fd = Register_fd_for_id(id);
    mu_assert(fd == 12, "Should get 12 for the fd.");

    int old_id = Register_id_for_fd(fd);
    mu_assert(old_id == id, "Wrong id for fd.");

    old_id = Register_disconnect(12);
    mu_assert(Register_fd_exists(12) == 0, "Didn't disconnect.");
    mu_assert(id == old_id, "Wrong id on disconnect.");

    // do it again to make sure we log invalid unregisters
    old_id = Register_disconnect(12);
    mu_assert(old_id == -1, "Should get an error.");
    return NULL;
}

char *test_Register_ping()
{
    int id = Register_connect(12232, TEST_CONN_1);
    mu_assert(id >= 0, "Failed to connect 12232.");
    mu_assert(Register_fd_exists(12232) == TEST_CONN_1, "Didn't register.");

    mu_assert(Register_ping(12232), "Ping didn't work.");

    // attempt a double registration which should NOT work
    int new_id = Register_connect(12232, TEST_CONN_2);
    mu_assert(new_id != id, "Second register should get new id.");
    mu_assert(Register_fd_exists(12232) == TEST_CONN_2, "Should have different connection");

    id = Register_disconnect(12232);
    mu_assert(id != -1, "Didn't disconnect after some pings.");
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

