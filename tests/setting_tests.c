#include "minunit.h"
#include <setting.h>

char *test_Setting_add_get()
{
    int rc = Setting_add("TEST1", "2345");
    mu_assert(rc == 0, "Failed to add TEST1");

    rc = Setting_add("TEST2", "Hello");
    mu_assert(rc == 0, "Failed to add TEST2");

    bstring t1 = Setting_get_str("TEST1", NULL);
    debug("Got t1: %s", bdata(t1));
    mu_assert(t1 != NULL, "Got the default.");
    mu_assert(biseqcstr(t1, "2345"), "Failed to get TEST1.");

    int t1_int = Setting_get_int("TEST1", 999);
    debug("Got %d for the TEST1", t1_int);
    mu_assert(t1_int == 2345, "Wrong value for TEST1 as int.");

    int t3_int = Setting_get_int("TEST5", 999);
    mu_assert(t3_int == 999, "Should get the default.");

    return NULL;
}

char *test_Setting_destroy()
{
    Setting_destroy();

    return NULL;
}

char * all_tests() {
    mu_suite_start();

    mu_run_test(test_Setting_add_get);
    mu_run_test(test_Setting_destroy);

    return NULL;
}

RUN_TESTS(all_tests);

