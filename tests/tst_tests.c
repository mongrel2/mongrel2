#include "minunit.h"
#include <adt/tst.h>
#include <string.h>
#include <assert.h>

FILE *LOG_FILE = NULL;

tst_t *node = NULL;
char *valueA = "VALUEA";
char *valueB = "VALUEB";
char *value2 = "VALUE2";
char *reverse = "VALUER";
int traverse_count = 0;


char *test_tst_insert() 
{
    node = tst_insert(node, "TEST", strlen("TEST"), valueA);
    mu_assert(node != NULL, "Failed to insert into tst.");

    node = tst_insert(node, "TEST", strlen("TEST"), valueB);
    mu_assert(node != NULL, "Failed to insert into tst with same name.");

    node = tst_insert(node, "TEST2", strlen("TEST2"), value2);
    mu_assert(node != NULL, "Failed to insert into tst with second name.");

    node = tst_insert(node, "TSET", strlen("TSET"), reverse);
    mu_assert(node != NULL, "Failed to insert into tst with reverse name.");

    return NULL;
}

char *test_tst_search()
{
    // tst returns the last one inserted
    void *res = tst_search(node, "TEST", strlen("TEST"));
    mu_assert(res == valueB, "Got the wrong value back.");

    // tst does not find if not exact
    res = tst_search(node, "TESTNOT", strlen("TESTNOT"));
    mu_assert(res == NULL, "Should not find anything.");

    return NULL;
}

char *test_tst_search_suffix()
{
    void *res = tst_search_suffix(node, "TEST", strlen("TEST"));
    debug("result: %p, expected: %p", res, reverse);
    mu_assert(res == reverse, "Got the wrong value.");

    res = tst_search_suffix(node, "TESTNOT", strlen("TESTNOT"));
    mu_assert(res == NULL, "Reverse search should find nothing.");

    res = tst_search_suffix(node, "EST", strlen("EST"));
    mu_assert(res == NULL, "Reverse search should find nothing on suffix.");

    return NULL;
}

void tst_traverse_test_cb(void *value, void *data)
{
    assert(value != NULL && "Should not get NULL value.");
    assert(data == valueA && "Expecting valueA as the data.");
    traverse_count++;
}

char *test_tst_traverse()
{
    traverse_count = 0;
    tst_traverse(node, tst_traverse_test_cb, valueA);
    debug("traverse count is: %d", traverse_count);
    mu_assert(traverse_count == 3, "Didn't find 3 keys.");

    return NULL;
}

char *test_tst_collect()
{
    list_t *found = tst_collect(node, "TE", 2, NULL, NULL);
    debug("collect found %d values", list_count(found));

    mu_assert(list_count(found) == 2, "Didn't find 2 with prefix TE.");

    found = tst_collect(node, "T", 1, NULL, NULL);
    debug("collect found %d values", list_count(found));

    mu_assert(list_count(found) == 3, "Didn't find 3 with prefix T.");
    mu_assert(list_count(found) == list_count(found), "Found count doesn't match list count.");

    found = tst_collect(node, "XNOT", 4, NULL, NULL);
    debug("collect found %d values", list_count(found));

    mu_assert(list_count(found) == 0, "Should not find any with prefix XNOT.");

    return NULL;
}

char * all_tests() {
    mu_suite_start();

    mu_run_test(test_tst_insert);
    mu_run_test(test_tst_search);
    mu_run_test(test_tst_search_suffix);
    mu_run_test(test_tst_traverse);
    mu_run_test(test_tst_collect);

    return NULL;
}

RUN_TESTS(all_tests);

