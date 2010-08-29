#include "minunit.h"
#include <config/config.h>
#include <server.h>
#include <zmq.h>
#include <task/task.h>
#include <config/db.h>

FILE *LOG_FILE = NULL;

char *test_Config_load() 
{
    Config_init_db("tests/config.sqlite");
    Server *server = Config_load_server("AC1F8236-5919-4696-9D40-0F38DE9E5861");
    
    mu_assert(server != NULL, "Failed to load server");

    Server_destroy(server);
    Config_close_db();

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

