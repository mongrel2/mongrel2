#undef NDEBUG
#include "minunit.h"
#include <tnetstrings.h>
#include <request.h>

FILE *LOG_FILE = NULL;

char *test_start_push_done()
{
    bstring list_data = bfromcstr("one two three four");
    struct bstrList *list = bsplit(list_data, ' ');
    bdestroy(list_data);

    tns_start();
    tns_push("%!%!%!", "test", "test", "test");
    tns_push("%d%d", 100, 200);
    tns_push("%!%!", "test", "test");
    tns_push("%l", list);
    tns_pop(']');

    mu_assert(biseqcstr(
            tns_top(),
            "78:4:test,4:test,4:test,3:100#3:200#4:test,4:test,27:3:one,3:two,5:three,4:four,]]"),
            "Pushing a list fails.");

    tns_clear();

    tns_push("%!%d", "age", 100);
    tns_push("%!%!", "name", "Zed A. Shaw");
    tns_pop('}');

    mu_assert(biseqcstr(tns_top(),
                "34:3:age,3:100#4:name,11:Zed A. Shaw,}"), "Pushing a hash fails.");
    tns_done();

    bstrListDestroy(list);
    return NULL;
}

char *test_tnetstrings_encode() 
{
    bstring payload = bfromcstr("");
    bstring test_bstr = bfromcstr("testbstr");

    tnetstring_encode(payload, "%!%s%s%d%b",
            "test yo", NULL, test_bstr, 10234, 1);

    mu_assert(biseqcstr(payload, "7:test yo,0:~8:testbstr,5:10234#4:true!"), "Payload fails.");

    bstring list_data = bfromcstr("one two three four");
    struct bstrList *list = bsplit(list_data, ' ');
    bdestroy(list_data);

    btrunc(payload, 0);
    tnetstring_encode(payload, "%l", list);
    mu_assert(biseqcstr(payload, "27:3:one,3:two,5:three,4:four,]"), "List fails.");
    bstrListDestroy(list);

    Request *req = Request_create();
    Request_set(req, &HTTP_METHOD, &JSON_METHOD, 0);
    Request_set(req, &HTTP_METHOD, &XML_METHOD, 0);
    Request_set(req, &HTTP_VERSION, &XML_METHOD, 0);

    btrunc(payload, 0);
    tnetstring_encode(payload, "%h", req->headers);

    mu_assert(biseqcstr(payload, "42:7:VERSION,3:XML,6:METHOD,13:4:JSON,3:XML,]}"), "Hash fails.");

    Request_destroy(req);
    bdestroy(payload);
    bdestroy(test_bstr);
    return NULL;
}

char * all_tests() {
    Request_init();
    mu_suite_start();

    mu_run_test(test_tnetstrings_encode);

    int i = 0;
    for(i = 0; i < 30000; i++) {
        mu_run_test(test_start_push_done);
    }

    return NULL;
}

RUN_TESTS(all_tests);

