#include "minunit.h"
#include <http11/http11_parser.h>
#include <glob.h>
#include <bstring.h>

void debug_element_cb(void *data, const char *at, size_t length)
{
    (void)data;
    (void)at;
    (void)length;
}

void debug_field_cb(void *data, const char *field, size_t flen, const char *value, size_t vlen)
{
    (void)data;
    (void)field;
    (void)flen;
    (void)value;
    (void)vlen;
}

http_parser setup_parser()
{
    http_parser p;
    p.http_field = debug_field_cb;
    p.request_method = debug_element_cb;
    p.request_uri = debug_element_cb;
    p.fragment = debug_element_cb;
    p.request_path = debug_element_cb;
    p.query_string = debug_element_cb;
    p.http_version = debug_element_cb;
    p.header_done = debug_element_cb;

    http_parser_init(&p);

    return p;
}

char *test_http11_parser_basics() 
{
    http_parser p = setup_parser();
    int rc = 0;

    rc = http_parser_finish(&p);
    mu_assert(rc == 0, "Should NOT be finished if nothing parsed.");

    rc = http_parser_has_error(&p);
    mu_assert(rc == 0, "Should not have an error at the beginning.");

    rc = http_parser_is_finished(&p);
    mu_assert(rc == 0, "Should not be finished since never handed anything.");
    return NULL;
}

char *test_parser_thrashing()
{
    glob_t test_files;
    unsigned int i = 0;
    int nparsed = 0;
    int delta = 0;
    int tests_run = 0;
    int execs_run = 0;
    int unfinished = 0;
    int errors = 0;

    int rc = glob("tests/and_suite/*", 0, NULL, &test_files);
    mu_assert(rc == 0, "Failed to glob file sin tests/and_suite/*");

    for(i = 0; i < test_files.gl_pathc; i++) {
        FILE *infile = fopen(test_files.gl_pathv[i], "r");
        mu_assert(infile != NULL, "Failed to open test file.");

        bstring data = bread((bNread)fread, infile);
        fclose(infile);
        mu_assert(data != NULL, "Failed to read test file.");

        tests_run++;

        http_parser p = setup_parser();

        nparsed = 0;
        delta = 0;

        while(nparsed < blength(data)) {
            debug("json PARSING: %d of %d at %s", nparsed, blength(data), bdataofs(data, nparsed));

            delta = http_parser_execute(&p, bdata(data), blength(data), nparsed);
            execs_run++;

            if(delta == 0) { break; }

            if(!http_parser_finish(&p)) {
                unfinished++;
            }

            nparsed += delta;

            if(http_parser_has_error(&p)) {
                errors++;
            }

            debug("TEST %s results: delta %d, has_error: %d, is_finished: %d",
                    test_files.gl_pathv[i],
                    nparsed, http_parser_has_error(&p), http_parser_is_finished(&p));

            http_parser_init(&p);  // reset for the next try
        }
    }

    debug("HTTP PARSING: tests_run: %d, execs_run: %d, unfinished: %d, errors: %d",
            tests_run, execs_run, unfinished, errors);

    return NULL;
}


char * all_tests() {
    mu_suite_start();

    mu_run_test(test_http11_parser_basics);
    mu_run_test(test_parser_thrashing);

    return NULL;
}

RUN_TESTS(all_tests);

