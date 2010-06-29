#include "minunit.h"
#include <adt/tst.h>
#include <string.h>
#include <assert.h>
#include <bstring.h>


FILE *LOG_FILE = NULL;

tst_t *node = NULL;
char *valueA = "VALUEA";
char *valueB = "VALUEB";
char *value2 = "VALUE2";
char *reverse = "VALUER";
int traverse_count = 0;

bstring test1;
bstring test2;
bstring test3;

char *test_tst_insert() 
{
    node = tst_insert(node, bdata(test1), blength(test1), valueA);
    mu_assert(node != NULL, "Failed to insert into tst.");

    node = tst_insert(node, bdata(test2), blength(test2), value2);
    mu_assert(node != NULL, "Failed to insert into tst with second name.");

    node = tst_insert(node, bdata(test3), blength(test3), reverse);
    mu_assert(node != NULL, "Failed to insert into tst with reverse name.");

    return NULL;
}

char *test_tst_search()
{
    // tst returns the last one inserted
    void *res = tst_search(node, bdata(test1), blength(test1));
    mu_assert(res == valueA, "Got the wrong value back, should get A not B.");

    // tst does not find if not exact
    res = tst_search(node, "TESTNOT", strlen("TESTNOT"));
    mu_assert(res == NULL, "Should not find anything.");

    return NULL;
}

char *test_tst_search_suffix()
{
    void *res = tst_search_suffix(node, bdata(test1), blength(test1));
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
    list_t *found = tst_collect(node, "TE", 2, NULL);
    debug("collect found %d values", (int)list_count(found));

    mu_assert(list_count(found) == 3, "Didn't find 2 with prefix TE.");
    list_destroy_nodes(found);
    list_destroy(found);

    found = tst_collect(node, "T", 1, NULL);
    debug("collect found %d values", (int)list_count(found));
    mu_assert(list_count(found) == 3, "Didn't find 4 with prefix T.");
    list_destroy_nodes(found);
    list_destroy(found);

    found = tst_collect(node, "TEST2", 5, NULL);
    debug("collect found %d values", (int)list_count(found));
    mu_assert(list_count(found) == 1, "Didn't find 1 with prefix TEST2.");
    list_destroy_nodes(found);
    list_destroy(found);

    found = tst_collect(node, "XNOT", 4, NULL);
    mu_assert(list_count(found) == 0, "Should not find any with prefix XNOT.");
    list_destroy_nodes(found);
    list_destroy(found);

    return NULL;
}

char *test_tst_destroy()
{
    tst_destroy(node);

    return NULL;
}

char * all_tests() {
    mu_suite_start();

    test1 = bfromcstr("TEST");
    test2 = bfromcstr("TEST2");
    test3 = bfromcstr("TSET");

    mu_run_test(test_tst_insert);
    mu_run_test(test_tst_search);
    mu_run_test(test_tst_search_suffix);
    mu_run_test(test_tst_traverse);
    mu_run_test(test_tst_collect);
    mu_run_test(test_tst_destroy);

    bdestroy(test1);
    bdestroy(test2);
    bdestroy(test3);

    return NULL;
}

RUN_TESTS(all_tests);

