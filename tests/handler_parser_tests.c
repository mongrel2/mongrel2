#include "minunit.h"
#include <handler_parser.h>
#include <bstring.h>

int test_parse(const char *test, int target_count)
{
    HandlerParser *parser = HandlerParser_create(128);

    bstring T1 = bfromcstr(test);

    int rc = HandlerParser_execute(parser, bdata(T1), blength(T1));

    bdestroy(T1);

    return rc == 1 && target_count == (int)parser->target_count;
}

#define TEST(T, C, M) mu_assert(test_parse(T, C) == 1, "Failed to parse: " #M);
#define TEST_FAIL(T, C, M) mu_assert(test_parse(T, C) == 0, "SHOULD fail to parse: " #M);

char *test_HandlerParser_execute() 
{
    TEST("5a9a6354-fc33-4468-8ccd-5d736737dad7 2:12, The body", 1, "T1");
    TEST("5a9a6354-fc33-4468-8ccd-5d736737dad7 11:0 1 2 3 4 5, The body", 6, "T2");
    TEST("5a9a6354-fc33-4468-8ccd-5d736737dad7 5:12 34, Another body.", 2, "T3");
    TEST("5a9a6354fc3344688ccd5d736737dad7 5:12 34, ", 2, "EMPTY");

    TEST_FAIL("this.is.wrong 5:12 34, ", 2, "BAD UUID");
    TEST_FAIL("5a9a6354fc3344688ccd5d736737dad7 10:12 34, ", 2, "TOO LONG NETSTRING");
    TEST_FAIL("5a9a6354fc3344688ccd5d736737dad7 3:12 34, ", 2, "TOO SHORT NETSTRING");
    TEST_FAIL("5a9a6354fc3344688ccd5d736737dad7 5:12 34,", 2, "NO TRAILING SPACE");
    TEST_FAIL(" 5:12 34,", 2, "NO UUID");

    return NULL;
}



char * all_tests() {
    mu_suite_start();

    mu_run_test(test_HandlerParser_execute);

    return NULL;
}

RUN_TESTS(all_tests);

