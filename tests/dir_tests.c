#include "minunit.h"
#include <dir.h>
#include <string.h>

FILE *LOG_FILE = NULL;

char *test_Dir_find_file() 
{
    bstring ctype = NULL;

    FileRecord *file = Dir_find_file(bfromcstr("tests/sample.json"), 
            ctype = bfromcstr("text/plain"));

    mu_assert(file != NULL, "Failed to find the file.");

    FileRecord_destroy(file);
    bdestroy(ctype);

    return NULL;
}


char *test_Dir_resolve_file()
{
    Dir *test = Dir_create("tests/", "/", "index.html", "test/plain");
    mu_assert(test != NULL, "Failed to make test dir.");

    FileRecord *rec = Dir_resolve_file(test, "/sample.json");
    mu_assert(rec != NULL, "Failed to resolve file that should be there.");

    FileRecord_destroy(rec);
    Dir_destroy(test);
    
    return NULL;
}

char * all_tests() {
    mu_suite_start();

    mu_run_test(test_Dir_find_file);

    return NULL;
}

RUN_TESTS(all_tests);

