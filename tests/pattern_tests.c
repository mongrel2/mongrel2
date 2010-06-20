#include "minunit.h"
#include <pattern.h>
#include <string.h>

FILE *LOG_FILE = NULL;

char *test_pattern_match() 
{
    const char *m = pattern_match("ZED", strlen("ZED"), "ZED");
    mu_assert(m != NULL, "Should match.");

    m = pattern_match("ZED", strlen("ZED"), "FOO");
    mu_assert(m == NULL, "Should not match.");

    m = pattern_match("ZEEEED", strlen("ZEEEED"), "ZE*D");
    mu_assert(m != NULL, "Should match.");

    m = pattern_match("ZEEEED", strlen("ZEEEED"), "ZX*D");
    mu_assert(m == NULL, "Should not match.");

    m = pattern_match("ZEEEED", strlen("ZEEEED"), "Z.*D");
    mu_assert(m != NULL, "Should match.");
    
    m = pattern_match("ZXXXXD", strlen("ZXXXXD"), "ZE.*D");
    mu_assert(m == NULL, "Should not match.");

    m = pattern_match("ZED", strlen("ZED"), ".*D$");
    mu_assert(m != NULL, "Should match.");

    m = pattern_match("ZED", strlen("ZED"), ".*X$");
    mu_assert(m == NULL, "Should not match.");

    return NULL;
}


char *test_pattern_tst_collect()
{
    char *valueA = "ZEDVALUE";
    char *valueB = "FRANKVALUE";
    char *valueC = "ALANVALUE";

    tst_t *node = tst_insert(NULL, "ZED", strlen("ZED"), valueA);
    node = tst_insert(node, "ZEDFRANK", strlen("ZEDFRANK"), valueB);
    node = tst_insert(node, "ZEDALAN", strlen("ZEDALAN"), valueC);

    list_t *results = pattern_tst_collect(node, "ZED");
    debug("Got back %d found.", list_count(results));

    mu_assert(results, "Didn't find zed normally.");

    results = pattern_tst_collect(node, "ZED.*K");
    debug("Got back %d found.", list_count(results));

    mu_assert(results, "Didn't find zed frank pattern.");

    return NULL;
}


char * all_tests() {
    mu_suite_start();

    mu_run_test(test_pattern_match);
    mu_run_test(test_pattern_tst_collect);

    return NULL;
}

RUN_TESTS(all_tests);


