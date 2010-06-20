#include "minunit.h"
#include <pattern.h>

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


char * all_tests() {
    mu_suite_start();

    mu_run_test(test_pattern_match);

    return NULL;
}

RUN_TESTS(all_tests);


