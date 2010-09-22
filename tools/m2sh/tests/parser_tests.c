#include "minunit.h"
#include "config_file.h"
#include "parser.h"
#include "ast.h"
#include <stdio.h>
#include <bstring.h>
#include <assert.h>

FILE *LOG_FILE = NULL;
int CB_FIRED = 0;

int check_callback(hash_t *parent, Value *val)
{
    assert(parent && "Should get a parent.");
    assert(val && "Should get a val.");
    CB_FIRED = 1;
    return 0;
}

char *test_parser() 
{
    hash_t *settings = Parse_config_file("tests/sample.conf");
    mu_assert(settings, "Error parsing.");

    AST_walk(settings, check_callback);
    AST_destroy(settings);

    return NULL;
}



char * all_tests() {
    mu_suite_start();

    mu_run_test(test_parser);

    return NULL;
}

RUN_TESTS(all_tests);

