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
    Dir *test = Dir_create("tests/", "/", "sample.html", "test/plain");
    mu_assert(test != NULL, "Failed to make test dir.");

    FileRecord *rec = Dir_resolve_file(test, bfromcstr("/sample.json"));
    mu_assert(rec != NULL, "Failed to resolve file that should be there.");

    rec = Dir_resolve_file(test, bfromcstr("/"));
    mu_assert(rec != NULL, "Failed to find default file.");

    rec = Dir_resolve_file(test, bfromcstr("/../../../../../etc/passwd"));
    mu_assert(rec == NULL, "HACK! should not find this.");
    
    test = Dir_create("foobar/", "/", "sample.html", "test/plan");
    mu_assert(test != NULL, "Failed to make the failed dir.");

    rec = Dir_resolve_file(test, bfromcstr("/sample.json"));
    mu_assert(test == NULL, "Should not get something from a bad base directory.");

    return NULL;
}



char * all_tests() {
    mu_suite_start();

    mu_run_test(test_Dir_find_file);

    return NULL;
}

RUN_TESTS(all_tests);

