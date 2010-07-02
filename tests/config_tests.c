#include "minunit.h"
#include <config/config.h>
#include <server.h>
#include <zmq.h>
#include <task/task.h>
#include <config/db.h>

FILE *LOG_FILE = NULL;

char *test_Config_load() 
{
    list_t *servers = Config_load_servers("tests/config.sqlite", "localhost");

    mu_assert(servers != NULL, "Should get a server list, is mongrel2 running already?");
    mu_assert(list_count(servers) == 1, "Failed to load the server.");

    Server_destroy(lnode_get(list_first(servers)));
    DB_close();

    return NULL;
}


char * all_tests() 
{
    mu_suite_start();

    Server_init();

    mu_run_test(test_Config_load);

    zmq_term(ZMQ_CTX);

    return NULL;
}

RUN_TESTS(all_tests);

