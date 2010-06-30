#include "minunit.h"
#include <dir.h>
#include <string.h>

FILE *LOG_FILE = NULL;

char *test_Dir_find_file() 
{
    size_t fsize;
    bstring t1;

    int fd = Dir_find_file(t1 = bfromcstr("tests/sample.json"), &fsize);
    mu_assert(fd >= 0, "Failed to find the file.");

    bdestroy(t1);

    return NULL;
}


char * all_tests() {
    mu_suite_start();

    mu_run_test(test_Dir_find_file);

    return NULL;
}

RUN_TESTS(all_tests);

