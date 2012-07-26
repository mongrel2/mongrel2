#include "minunit.h"
#include "filter.h"
#include "connection.h"
#include "tnetstrings_impl.h"

char *test_Filter_load() 
{
    Server *srv = NULL;
    bstring load_path = bfromcstr("tests/filters/test_filter.so");

    int res = Filter_load(srv, load_path, tns_new_dict());
    mu_assert(res == 0, "Failed to load tests/filters/test_filter.so");
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


char *test_Filter_run_chain(){

	Filter_init(); /*We need a fresh new Filter storage*/
	Connection *conn = Connection_create(NULL, 0, 80, "");

    int res_filter_a = Filter_load(NULL, bfromcstr("tests/filters/test_filter_a.so"), tns_new_dict());
    mu_assert(res_filter_a == 0, "Failed to load tests/filters/test_filter_a.so");

    int res_filter_b = Filter_load(NULL, bfromcstr("tests/filters/test_filter_b.so"), tns_new_dict());
    mu_assert(res_filter_b == 0, "Failed to load tests/filters/test_filter_b.so");

    Filter_run(CONNECT, conn);

    /* filter_a sums 2 on conn->rport, filter_b sums 8*/
    mu_assert(conn->rport == 90, "Not all filters was run, rport not correctly changed");

	return NULL;
}

char *test_Filter_stop_filter_chain(){
	Filter_init(); /*We need a fresh new Filter storage*/
	Connection *conn = Connection_create(NULL, 0, 80, "");

    int res_filter_a = Filter_load(NULL, bfromcstr("tests/filters/test_filter_a.so"), tns_new_dict());
    mu_assert(res_filter_a == 0, "Failed to load tests/filters/test_filter_a.so");

    int res_filter_c = Filter_load(NULL, bfromcstr("tests/filters/test_filter_c.so"), tns_new_dict());
    mu_assert(res_filter_c == 0, "Failed to load tests/filters/test_filter_c.so");

    int res_filter_b = Filter_load(NULL, bfromcstr("tests/filters/test_filter_b.so"), tns_new_dict());
    mu_assert(res_filter_b == 0, "Failed to load tests/filters/test_filter_b.so");

    int next = Filter_run(CONNECT, conn);
    mu_assert(next == CLOSE, "Last filter not run correctly");

    /* Only filter_a and filter_c should run. */
    mu_assert(conn->rport == 87, "Not all filters should be run");

	return NULL;
}



char * all_tests() {
    mu_suite_start();

    mu_run_test(test_Filter_load);
    mu_run_test(test_Filter_run);
    mu_run_test(test_Filter_run_chain);
    mu_run_test(test_Filter_stop_filter_chain);

    return NULL;
}

RUN_TESTS(all_tests);

