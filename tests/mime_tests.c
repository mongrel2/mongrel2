#include "minunit.h"
#include <mime.h>

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
    bstring type;
    bstring t1, t2, t3, t4, t5;

    type = MIME_match_ext(t1 = bfromcstr("test.html"), NULL);
    mu_assert(type, "Didn't find .html file.");
    mu_assert(biseqcstr(type, "text/html"), "Wrong type returned for.html.");

    type = MIME_match_ext(t2 = bfromcstr("test.json"), NULL);
    mu_assert(type, "Didn't find .json file.");
    debug("GOT json type: %s", bdata(type));
    mu_assert(biseqcstr(type, "application/javascript"), "Wrong type returned for .json.");

    type = MIME_match_ext(t3 = bfromcstr("test.crap"), NULL);
    mu_assert(!type, "Should not find unknown types.");

    type = MIME_match_ext(t4 = bfromcstr("test.crap"), t5 = bfromcstr("text/plain"));
    mu_assert(type, "Should get the default type.");
    mu_assert(biseqcstr(type, "text/plain"), "Wrong type returned for.html.");

    bdestroy(t1);
    bdestroy(t2);
    bdestroy(t3);
    bdestroy(t4);
    bdestroy(t5);
    return NULL;
}

char *test_MIME_destroy()
{
    MIME_destroy();
    return NULL;
}


char * all_tests() {
    mu_suite_start();

    mu_run_test(test_MIME_failures);
    mu_run_test(test_MIME_ops);
    mu_run_test(test_MIME_destroy);

    return NULL;
}

RUN_TESTS(all_tests);

