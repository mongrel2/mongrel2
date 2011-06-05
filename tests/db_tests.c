#include "minunit.h"
#include <config/db.h>
#include <dbg.h>

char *test_DB_init() 
{
    mu_assert(DB_init("tests/empty.sqlite") == 0, "Database init failed.");

    return NULL;
}

char *test_DB_close()
{
    DB_close();

    return NULL;
}

struct tagbstring EMPTY_RESULT = bsStatic("0:~");

int check_result(tns_value_t *res, bstring expecting)
{
    char *val = NULL;
    size_t len = 0;

    check(res != NULL, "Got NULL for result.");
    val = tns_render(res, &len);
    check(val != NULL, "Failed to render value result.");
    check(bisstemeqblk(expecting, val, len),
            "Wrong result expecting: '%s' got \"%.*s\"",
            bdata(expecting), (int)len, val);

    free(val);
    tns_value_destroy(res);
    return 1;

error:
    free(val);
    tns_value_destroy(res);
    return 0;
}

char *test_DB_exec()
{
    DB_init("tests/empty.sqlite");
    tns_value_t *res = NULL;
    int cols = 0;
    int rows = 0;

    res = DB_exec("DROP TABLE IF EXISTS testing");
    mu_assert(check_result(res, &EMPTY_RESULT), "Drop table failed.");

    res = DB_exec("CREATE TABLE testing (name TEXT, age INT)");
    rows = DB_counts(res, &cols);
    mu_assert(rows == 0, "Should be 0 length result.");
    mu_assert(cols == 0, "Should be 0 cols as well.");
    mu_assert(check_result(res, &EMPTY_RESULT), "Create table failed");

    res = DB_exec("INSERT INTO testing VALUES ('Zed',35)");
    mu_assert(check_result(res, &EMPTY_RESULT), "Insert failed");

    bstring select_result = bfromcstr("15:11:3:Zed,2:35#]]");
    res = DB_exec("SELECT * FROM testing");
    mu_assert(tns_get_type(res) == tns_tag_list, "Wrong type, should get a list from select.");

    // test out the simplified get commands
    
    rows = DB_counts(res, &cols);
    mu_assert(rows != -1, "Failed to get the counts of the result.");
    mu_assert(rows == 1, "Should get one row.");
    mu_assert(cols == 2, "Should get two cols.");

    bstring name = DB_get_as(res, 0, 0, string);
    mu_assert(biseqcstr(name, "Zed"), "Wrong value for column 0.");

    int age = DB_get_as(res, 0, 1, number);
    debug("GOT AGE: %d", age);
    mu_assert(age == 35, "Wrong value for column 1.");


    int i = 0;
    int j = 0;
    for(i = 0; i < rows; i++) {
        for(j = 0; j < cols; j++) {
            tns_value_t *el = DB_get(res, i, j);
            mu_assert(tns_get_type(el) != tns_tag_invalid, "Value should not be invalid(NULL)");
        }
    }

    // check that the error handling works well
    mu_assert(DB_get(res, 100, 0) == NULL, "Should not work with insane rows.");
    mu_assert(DB_get(res, 0, 100) == NULL, "Should not work with insane cols.");
    mu_assert(DB_get(res, 32, 87) == NULL, "Should not work with insane cols and rows.");

    mu_assert(check_result(res, select_result), "Select failed.");
    bdestroy(select_result);

    // finally make sure the counts and get handle NULL pointers
    mu_assert(DB_counts(NULL, &cols) == -1, "Should abort on NULL.");
    mu_assert(DB_get(NULL, 0, 0) == NULL, "Should abort on NULL.");

    return NULL;
}

char * all_tests() {
    mu_suite_start();

    mu_run_test(test_DB_init);
    mu_run_test(test_DB_close);
    mu_run_test(test_DB_exec);

    DB_close();

    return NULL;
}

RUN_TESTS(all_tests);

