#include "minunit.h"
#include <dir.h>
#include <string.h>

FILE *LOG_FILE = NULL;

char *test_Dir_find_file() 
{
    bstring t1;

    FileRecord *file = Dir_find_file(t1 = bfromcstr("tests/sample.json"));
    mu_assert(file != NULL, "Failed to find the file.");

    bdestroy(t1);
    FileRecord_destroy(file);

    return NULL;
}


char * all_tests() {
    mu_suite_start();

    mu_run_test(test_Dir_find_file);

    return NULL;
}

RUN_TESTS(all_tests);

