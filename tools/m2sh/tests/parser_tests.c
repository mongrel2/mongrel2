#include "minunit.h"
#include "config.h"
#include "parser.h"
#include "ast.h"
#include <stdio.h>
#include <bstring.h>

FILE *LOG_FILE = NULL;

char *test_parser() 
{
    hash_t *settings = Parse_config_file("tests/sample.conf");
    mu_assert(settings, "Error parsing.");

    AST_walk(settings);

    return NULL;
}



char * all_tests() {
    mu_suite_start();

    mu_run_test(test_parser);

    return NULL;
}

RUN_TESTS(all_tests);

