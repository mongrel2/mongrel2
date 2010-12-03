#undef NDEBUG

#include "minunit.h"
#include <request.h>
#include <headers.h>

FILE *LOG_FILE = NULL;

const char *RFC_822_TIME = "%a, %d %b %y %T %z";

char *test_Request_create() 
{
    int rc = 0;
    size_t nparsed = 0;

    Request_init();

    Request *req = Request_create();
    mu_assert(req != NULL, "Failed to create parser for request.");

    FILE *infile = fopen("tests/and_suite/ex_httpd_tst_16", "r");
    mu_assert(infile != NULL, "Failed to open test file.");

    bstring data = bread((bNread)fread, infile);
    fclose(infile);
    mu_assert(data != NULL, "Failed to read test file.");
    mu_assert(blength(data) > 0, "Nothing in that file.");

    Request_start(req);

    rc = Request_parse(req, bdata(data), blength(data), &nparsed);

    mu_assert(rc == 1, "It should parse.");
    mu_assert(nparsed > 0, "Should have parsed something.");

    bstring payload = Request_to_payload(req, &JSON_METHOD, 0, "", 0);
    debug("PAYLOAD IS: %s", bdata(payload));

    mu_assert(Request_get(req, &HTTP_IF_MODIFIED_SINCE) != NULL,
            "Should have an if-modified-since header.");
    mu_assert(req->host != NULL, "Should have Host header.");
    mu_assert(Request_get_date(req, &HTTP_IF_MODIFIED_SINCE, RFC_822_TIME) > 0, 
            "Wrong time from header.");

    mu_assert(Request_get_date(req, &HTTP_IF_UNMODIFIED_SINCE, RFC_822_TIME) == 0,
            "Unmodified since should be invalid.");

    mu_assert(Request_get_date(req, &HTTP_IF_NONE_MATCH, RFC_822_TIME) == 0,
            "None match shouldn't even be a date.");

    Request_start(req);

    Request_destroy(req);

    // test with null
    Request_destroy(NULL);

    return NULL;
}

struct tagbstring COOKIE_HEADER = bsStatic("cookie");
struct tagbstring EXPECTED_COOKIE_HEADER = bsStatic("JSON 0 / 97:{\"PATH\":\"/\",\"cookie\":[\"foo=bar\",\"test=yes; go=no\"],\"METHOD\":\"GET\",\"VERSION\":\"HTTP/1.0\",\"URI\":\"/\"},0:,");

char *test_Multiple_Header_Request() 
{
    int rc = 0;
    size_t nparsed = 0;

    Request_init();

    Request *req = Request_create();
    mu_assert(req != NULL, "Failed to create parser for request.");

    FILE *infile = fopen("tests/and_suite/ex_httpd_tst_21", "r");
    mu_assert(infile != NULL, "Failed to open test file.");

    bstring data = bread((bNread)fread, infile);
    fclose(infile);
    mu_assert(data != NULL, "Failed to read test file.");
    mu_assert(blength(data) > 0, "Nothing in that file.");

    Request_start(req);

    rc = Request_parse(req, bdata(data), blength(data), &nparsed);

    mu_assert(rc == 1, "It should parse.");
    mu_assert(nparsed > 0, "Should have parsed something.");

    mu_assert(Request_get(req, &COOKIE_HEADER) != NULL,
            "Should have an cookie header.");

    bstring payload = Request_to_payload(req, &JSON_METHOD, 0, "", 0);
    debug("PAYLOAD IS: %s", bdata(payload));

    mu_assert(bstrcmp(payload, &EXPECTED_COOKIE_HEADER) == 0,
            "Expected header not in correct format.");

    Request_destroy(req);

    return NULL;
}


char * all_tests() {
    mu_suite_start();

    mu_run_test(test_Request_create);
    mu_run_test(test_Multiple_Header_Request);

    return NULL;
}

RUN_TESTS(all_tests);

