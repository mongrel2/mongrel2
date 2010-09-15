#include "minunit.h"
#include "cli.h"
#include <stdio.h>
#include <bstring.h>

FILE *LOG_FILE = NULL;


char *test_parser() 
{
    Command cmd;
    bstring args;

    args = bfromcstr("m2sh stop -db 'config.sqlite' -name 'main'");
    mu_assert(cli_params_parse_args(args, &cmd) != -1, "Parse returned -1.");
    mu_assert(!cmd.error, "Parsing failed.");

    args = bfromcstr("m2sh stop -db 'config.sqlite' -name 'main' -now --murder");
    mu_assert(cli_params_parse_args(args, &cmd) != -1, "Parse returned -1.");
    mu_assert(!cmd.error, "Parsing failed.");

    args = bfromcstr("m2sh something -db 'config.sqlite' one two three 4 \"whatever\" --when 12");
    mu_assert(cli_params_parse_args(args, &cmd) != -1, "Parse returned -1.");
    mu_assert(!cmd.error, "Parsing failed.");

    args = bfromcstr("m2sh start -db config#sqlite -name main");
    mu_assert(cli_params_parse_args(args, &cmd) != -1, "Parse returned -1.");
    mu_assert(!cmd.error, "Parsing failed.");

    return NULL;
}

char* all_tests() {
    mu_suite_start();

    mu_run_test(test_parser);

    return NULL;
}

RUN_TESTS(all_tests);

