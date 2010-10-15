#include "minunit.h"
#include <dir.h>
#include <string.h>

FILE *LOG_FILE = NULL;

char *test_Dir_find_file() 
{
    bstring ctype = NULL;

    FileRecord *file = Dir_find_file(bfromcstr("tests/sample.json"), 
            ctype = bfromcstr("text/plain"));

    mu_assert(file != NULL, "Failed to find the file.");

    FileRecord_destroy(file);
    bdestroy(ctype);

    return NULL;
}


char *test_Dir_resolve_file()
{
    Dir *test = Dir_create("tests/", "sample.html", "test/plain");
    mu_assert(test != NULL, "Failed to make test dir.");

    FileRecord *rec = Dir_resolve_file(test, bfromcstr("/"), bfromcstr("/sample.json"));
    mu_assert(rec != NULL, "Failed to resolve file that should be there.");

    rec = Dir_resolve_file(test, bfromcstr("/"), bfromcstr("/"));
    mu_assert(rec != NULL, "Failed to find default file.");

    rec = Dir_resolve_file(test, bfromcstr("/"), bfromcstr("/../../../../../etc/passwd"));
    mu_assert(rec == NULL, "HACK! should not find this.");
   
    Dir_destroy(test);

    test = Dir_create("foobar/", "sample.html", "test/plan");
    mu_assert(test != NULL, "Failed to make the failed dir.");

    rec = Dir_resolve_file(test, bfromcstr("/"), bfromcstr("/sample.json"));
    mu_assert(rec == NULL, "Should not get something from a bad base directory.");

    Dir_destroy(test);

    return NULL;
}

const char *REQ_PATTERN = "%s %s HTTP/1.1\r\n\r\n";

Request *fake_req(const char *method, const char *prefix, const char *path)
{
    int rc = 0;
    size_t nparsed = 0;
    Request *req = Request_create();
    Request_start(req);

    bstring p = bfromcstr(path);
    bstring rp = bformat(REQ_PATTERN, method, bdata(p));

    rc = Request_parse(req, bdata(rp), blength(rp), &nparsed);
    req->prefix = bfromcstr(prefix);
    req->pattern = bfromcstr(prefix);

    check(rc != 0, "Failed to parse request.");
    check(nparsed == blength(rp), "Failed to parse all of request.");

    return req;

error:
    return NULL;
}


char *test_Dir_serve_file()
{
    int rc = 0;
    Request *req = NULL;

    Dir *test = Dir_create("tests/", "sample.html", "test/plain");

    Connection conn = {0};
    int zero_fd = open("/dev/null", O_WRONLY);
    conn.iob = IOBuf_create(1024, zero_fd, IOBUF_NULL);

    req = fake_req("GET", "/", "/sample.json");
    rc = Dir_serve_file(test, req, &conn);
    // TODO: different platforms barf on sendfile for different reasons
    // mu_assert(req->response_size > -1, "Should serve the /sample.json");

    req = fake_req("HEAD", "/", "/sample.json");
    rc = Dir_serve_file(test, req, &conn);
    mu_assert(rc == 0, "Should serve the HEAD of /sample.json");

    req = fake_req("POST", "/", "/sample.json");
    rc = Dir_serve_file(test, req, &conn);
    mu_assert(rc == -1, "POST should pass through but send an error.");

    return NULL;
}

char * all_tests() {
    mu_suite_start();

    mu_run_test(test_Dir_find_file);
    mu_run_test(test_Dir_serve_file);
    mu_run_test(test_Dir_resolve_file);

    return NULL;
}

RUN_TESTS(all_tests);

