#include "minunit.h"
#include <proxy.h>
#include <stdlib.h>
#include <mem/halloc.h>


char *test_Proxy_create_destroy()
{
    Proxy *proxy = Proxy_create(bfromcstr("127.0.0.1"), 80);
    mu_assert(proxy != NULL, "Didn't make the proxy.");

    Proxy_destroy(proxy); 

    return NULL;
}

char * all_tests() {
    mu_suite_start();

    mu_run_test(test_Proxy_create_destroy);

    return NULL;
}

RUN_TESTS(all_tests);

