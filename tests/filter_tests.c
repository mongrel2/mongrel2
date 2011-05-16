#undef NDEBUG
#include "minunit.h"
#include <filter.h>

FILE *LOG_FILE = NULL;

char *test_Filter_load() 
{
    Server *srv = NULL;
    bstring load_path = bfromcstr("tools/filters/test_filter.so");

    int res = Filter_load(srv, load_path);
    mu_assert(res == 0, "Failed to load tools/filters/test_filter.so");
    mu_assert(Filter_activated(), "Filters not activated.");

    return NULL;
}

char *test_Filter_run()
{
    int next = Filter_run(HANDLER, NULL);
    debug("HANDLER returned: %d", next);
    
    mu_assert(next == CLOSE, "Wrong event for callback.");

    mu_assert(Filter_run(MSG_REQ, NULL) == MSG_REQ, "Should return same for non-registered.");

    return NULL;
}


char * all_tests() {
    mu_suite_start();

    mu_run_test(test_Filter_load);
    mu_run_test(test_Filter_run);

    return NULL;
}

RUN_TESTS(all_tests);

