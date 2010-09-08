#include "minunit.h"
#include "config.h"
#include "parser.h"
#include <stdio.h>
#include <bstring.h>

FILE *LOG_FILE = NULL;

char *test_parser() 
{
    int rc = Parse_config_file("tests/sample.conf");
    mu_assert(rc == 0, "Error parsing.");

    return NULL;
}



char * all_tests() {
    mu_suite_start();

    mu_run_test(test_parser);

    return NULL;
}

RUN_TESTS(all_tests);

