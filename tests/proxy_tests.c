#include "minunit.h"
#include <proxy.h>
#include <stdlib.h>

FILE *LOG_FILE = NULL;

char *test_ProxyConnect_create_destroy()
{
    ProxyConnect *conn = ProxyConnect_create(12, malloc(12), 12, 0);
    mu_assert(conn != NULL, "Didn't make the proxy connection.");

    ProxyConnect_destroy(conn);

    return NULL;
}


char *test_Proxy_create_destroy()
{
    Proxy *proxy = Proxy_create("127.0.0.1", 80);
    mu_assert(proxy != NULL, "Didn't make the proxy.");

    Proxy_destroy(proxy); 

    return NULL;
}

char *test_Proxy_connect()
{
    // hopefully this port isn't there so we error on connect
    Proxy *proxy = Proxy_create("127.0.0.1", 12345);
    mu_assert(proxy != NULL, "Didn't make the proxy.");

    Proxy_connect(proxy, 12, malloc(12), 12, 0);
    Proxy_destroy(proxy);

    return NULL;
}

char * all_tests() {
    mu_suite_start();

    mu_run_test(test_ProxyConnect_create_destroy);
    mu_run_test(test_Proxy_create_destroy);

    return NULL;
}

RUN_TESTS(all_tests);

