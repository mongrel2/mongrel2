#include "minunit.h"
#include "request.h"
#include "tnetstrings.h"
#include "headers.h"
#include <glob.h>
#include "register.h"
#include "connection.h"

static const char *RFC_822_TIME = "%a, %d %b %y %T";
Connection *conn=NULL;

char *test_Request_payloads()
{
    glob_t test_files;
    bstring fake_sender = bfromcstr("FAKESENDER");
    Request *req = Request_create();
    //Connection *conn=Connection_create(NULL,0,1,"");
    size_t nparsed = 0;
    unsigned int i = 0;
    int rc = glob("tests/and_suite/*", 0, NULL, &test_files);
    mu_assert(rc == 0, "Failed to glob file sin tests/and_suite/*");
    FILE *test_cases = fopen("tests/request_payloads.txt", "w");
    mu_assert(test_cases != NULL, "Failed to create the tests/request_payloads.txt file.");

    for(i = 0; i < test_files.gl_pathc; i++) {
        nparsed = 0;
        FILE *infile = fopen(test_files.gl_pathv[i], "r");
        mu_assert(infile != NULL, "Failed to open test file.");

        bstring data = bread((bNread)fread, infile);
        fclose(infile);
        mu_assert(data != NULL, "Failed to read test file.");

        Request_start(req);
        rc = Request_parse(req, bdata(data), blength(data), &nparsed);

        if(rc == 1) {
            mu_assert(nparsed > 0, "Should have parsed something.");

            // TODO: fix this up so that we never get a null for path
            if(req->path == NULL) req->path = bfromcstr("/");

            bstring payload = Request_to_payload(req, fake_sender, 0, "", 0,conn, NULL);
            bconchar(payload, '\n');
            fwrite(payload->data, blength(payload), 1, test_cases);
            bdestroy(payload);

            payload = Request_to_tnetstring(req, fake_sender, 0, "", 0,conn, NULL);
            debug("TNETSTRING PAYLOAD: '%.*s'", blength(payload), bdata(payload));
            bconchar(payload, '\n');
            fwrite(payload->data, blength(payload), 1, test_cases);
            bdestroy(payload);
        }

        bdestroy(data);
    }

    globfree(&test_files);
    fclose(test_cases);
    bdestroy(fake_sender);
    Request_destroy(req);
    return NULL;
}

char *test_Request_speeds()
{
    int i = 0;
    FILE *infile = fopen("tests/request_payloads.txt", "r");
    mu_assert(infile != NULL, "Failed to open the tests/request_payloads.txt test file.");
    struct bstrList *list = bstrListCreate();
    bstrListAlloc(list, 300);

    // load up all the ones we can
    for(i = 0; i < 300; i++, list->qty++) {
        bstring sender = bgets((bNgetc)fgetc, infile, ' ');
        bstring conn_id = bgets((bNgetc)fgetc, infile, ' ');
        bstring path = bgets((bNgetc)fgetc, infile, ' ');
        list->entry[i] = bgets((bNgetc)fgetc, infile, '\n');

        // stop if we didn't read anything
        if(!(sender && conn_id && path)) break;

        bdestroy(sender);
        bdestroy(conn_id);
        bdestroy(path);
    }

    fclose(infile);


    // now rip through and parse them for speed test
    int j = 0;
    for(j = 0; j < 200; j++) {
        for(i = 0; i < list->qty; i++) {
            tns_value_t *val = tns_parse(bdata(list->entry[i]), blength(list->entry[i]), NULL);
            mu_assert(val->type == tns_tag_string || val->type == tns_tag_dict, "Got an invalid data chunk from file.");

            size_t len = 0;
            char *payload = tns_render(val, &len);
            mu_assert(len > 0, "Failed to render the payload.");

            free(payload);
            tns_value_destroy(val);
        }
    }

    bstrListDestroy(list);
    return NULL;
}

char *test_Request_create() 
{
    int rc = 0;
    size_t nparsed = 0;
    bstring fake_sender = bfromcstr("FAKESENDER");

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

    bstring payload = Request_to_payload(req, fake_sender, 0, "", 0,conn, NULL);
    debug("PAYLOAD IS: %s", bdata(payload));
    bdestroy(payload);

    payload = Request_to_tnetstring(req, fake_sender, 0, "", 0,conn, NULL);
    debug("TNETSTRING PAYLOAD: '%.*s'", blength(payload), bdata(payload));

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
    bdestroy(payload);
    bdestroy(fake_sender);
    bdestroy(data);

    return NULL;
}

struct tagbstring COOKIE_HEADER = bsStatic("cookie");
struct tagbstring EXPECTED_COOKIE_HEADER = bsStatic("JSON 1 / 134:{\"PATH\":\"/\",\"cookie\":[\"foo=bar\",\"test=yes; go=no\"],\"METHOD\":\"GET\",\"VERSION\":\"HTTP/1.0\",\"URI\":\"/\",\"URL_SCHEME\":\"http\",\"REMOTE_ADDR\":\"\"},0:,");

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

    bstring payload = Request_to_payload(req, &JSON_METHOD, 0, "", 0,conn, NULL);
    debug("PAYLOAD IS: %s", bdata(payload));

    mu_assert(bstrcmp(payload, &EXPECTED_COOKIE_HEADER) == 0,
            "Expected header not in correct format.");

    Request_destroy(req);
    bdestroy(payload);
    bdestroy(data);

    return NULL;
}


char * all_tests() {
    mu_suite_start();
    Register_init();

    // some evil hackery to mock out a registration so that the payload functions work
    conn = Connection_create(NULL,0,1,"");
    /*Connection *conn = calloc(sizeof(Connection), 1);
    conn->iob = NULL;
    conn->type = CONN_TYPE_HTTP;*/
    Register_connect(0, conn);

    mu_run_test(test_Request_create);
    mu_run_test(test_Multiple_Header_Request);
    mu_run_test(test_Request_payloads);
    mu_run_test(test_Request_speeds);

    return NULL;
}

RUN_TESTS(all_tests);

