#include "minunit.h"
#include <mime.h>

FILE *LOG_FILE = NULL;

char *test_MIME_failures() 
{
    int rc = MIME_add_type("txt", "badtype");
    mu_assert(rc == -1, "Should not accept extension with '.'");
    rc = MIME_add_type("", "badtype");
    mu_assert(rc == -1, "Should reject empty ext.");
    rc = MIME_add_type(".txt", "");
    mu_assert(rc == -1, "Should reject empty MIME type.");

    return NULL;
}


char *test_MIME_ops()
{
    int rc = MIME_add_type(".txt", "text/plain");
    mu_assert(rc == 0, "Failed to add .txt");

    rc = MIME_add_type(".txt", "badtype");
    mu_assert(rc == -1, "Should not add duplicates");

    rc = MIME_add_type(".json", "application/javascript");
    mu_assert(rc == 0, "Failed to add .json");

    rc = MIME_add_type(".html", "text/html");
    mu_assert(rc == 0, "Failed to add .html");

    // now test that we can get them

    const char *type = NULL;

    type = MIME_match_ext("test.html", strlen("test.html"), NULL);
    mu_assert(type, "Didn't find .html file.");
    mu_assert(strcmp("text/html", type) == 0, "Wrong type returned for.html.");

    type = MIME_match_ext("test.json", strlen("test.json"), NULL);
    mu_assert(type, "Didn't find .json file.");
    mu_assert(strcmp("application/javascript", type) == 0, "Wrong type returned for .json.");

    type = MIME_match_ext("test.crap", strlen("test.crap"), NULL);
    mu_assert(!type, "Should not find unknown types.");

    type = MIME_match_ext("test.crap", strlen("test.crap"), "text/plain");
    mu_assert(type, "Should get the default type.");
    mu_assert(strcmp("text/plain", type) == 0, "Wrong type returned for.html.");
    return NULL;
}


char * all_tests() {
    mu_suite_start();

    mu_run_test(test_MIME_failures);
    mu_run_test(test_MIME_ops);

    return NULL;
}

RUN_TESTS(all_tests);

