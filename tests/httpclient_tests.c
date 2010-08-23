
#include "minunit.h"
#include <http11/httpclient_parser.h>
#include <glob.h>
#include <bstring.h>

FILE *LOG_FILE = NULL;


httpclient_parser setup_parser()
{
    httpclient_parser p;
    httpclient_parser_init(&p);
    p.http_field = NULL;
    p.reason_phrase = NULL;
    p.status_code = NULL;
    p.chunk_size = NULL;
    p.http_version = NULL;
    p.header_done = NULL;
    p.last_chunk = NULL;

    return p;
}

char *test_httpclient_parser_basics() 
{
    httpclient_parser p = setup_parser();
    p.data = &p;
    int rc = 0;

    rc = httpclient_parser_finish(&p);
    mu_assert(rc == 0, "Client parser SHOULD be finished if nothing parsed.");

    rc = httpclient_parser_has_error(&p);
    mu_assert(rc == 0, "Should not have an error at the beginning.");

    rc = httpclient_parser_is_finished(&p);
    mu_assert(rc == 0, "Should not be finished since never handed anything.");
    return NULL;
}

bstring load_test_case(const char *path)
{
    FILE *infile = fopen(path, "r");
    check(infile != NULL, "Failed to open test file.");

    bstring data = bread((bNread)fread, infile);
    fclose(infile);
    check(data != NULL, "Failed to read test file.");

    return data;

error:
    if(infile) fclose(infile);
    return NULL;
}

char *test_parser_thrashing()
{
    glob_t test_files;
    int i = 0;
    int nparsed = 0;
    int tests_run = 0;
    int execs_run = 0;
    int unfinished = 0;
    int errors = 0;

    int rc = glob("tests/client_suite/*", 0, NULL, &test_files);
    mu_assert(rc == 0, "Failed to glob file sin tests/client_suite/*");

    for(i = 0; i < test_files.gl_pathc; i++) {
        debug("TESTING: %s", test_files.gl_pathv[i]);
        bstring data = load_test_case(test_files.gl_pathv[i]);
        mu_assert(data != NULL, "Failed to load test case.");

        tests_run++;

        httpclient_parser p = setup_parser();
        p.data = &p;

        nparsed = httpclient_parser_execute(&p, bdata(data), blength(data), 0);
        execs_run++;

        if(!httpclient_parser_finish(&p)) {
            unfinished++;
        }

        if(httpclient_parser_has_error(&p)) {
            errors++;
            debug("TEST %s results: delta %d, has_error: %d, is_finished: %d",
                    test_files.gl_pathv[i],
                    nparsed, httpclient_parser_has_error(&p), 
                    httpclient_parser_is_finished(&p));
        } else if(p.chunked) {
            int start = p.body_start;

            do {
                httpclient_parser_init(&p);
                
                nparsed = httpclient_parser_execute(&p, bdata(data), blength(data), start);
                mu_assert(p.body_start > start, "Didn't go past start.");
                mu_assert(p.chunked, "Should still be chunked.");
                start = p.body_start + p.content_len + 1;

                debug("CHUNK: length: %d, done: %d, start: %d",
                        p.content_len, p.chunks_done, start);
            } while(!p.chunks_done && p.content_len > 0 && !httpclient_parser_has_error(&p));

            mu_assert(p.chunks_done, "Should have chunks_done set.");
            mu_assert(p.content_len == 0, "Should have 0 content_len too.");
        }
    }

    debug("HTTP PARSING: tests_run: %d, execs_run: %d, unfinished: %d, errors: %d",
            tests_run, execs_run, unfinished, errors);

    return NULL;
}


char * all_tests() {
    mu_suite_start();

    mu_run_test(test_httpclient_parser_basics);
    mu_run_test(test_parser_thrashing);

    return NULL;
}

RUN_TESTS(all_tests);

