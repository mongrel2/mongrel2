#include "minunit.h"
#include <config/db.h>
#include <dbg.h>

FILE *LOG_FILE = NULL;

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


int query_callback(void *param, int cols, char **data, char **names)
{
    int i = 0;

    for(i = 0; i < cols; i++) {
        debug("QUERY RESULT: %s=%s", names[i], data[i]);
    }

    return 0;
}

char *test_DB_exec()
{
    DB_init("tests/empty.sqlite");

    int rc = DB_exec("DROP TABLE IF EXISTS testing", NULL, NULL);
    mu_assert(rc == 0, "DROP TABLE failed.");

    rc = DB_exec("CREATE TABLE testing (name TEXT, age INT)", NULL, NULL);
    mu_assert(rc == 0, "CREATE TABLE failed.");

    rc = DB_exec("INSERT INTO testing VALUES ('Zed',35)", NULL, NULL);
    mu_assert(rc == 0, "INSERT FAILED.");

    rc = DB_exec("SELECT * FROM testing", query_callback, NULL);
    mu_assert(rc == 0, "SELECT FAILED.");

    return NULL;
}

char * all_tests() {
    mu_suite_start();

    mu_run_test(test_DB_init);
    mu_run_test(test_DB_close);
    mu_run_test(test_DB_exec);

    return NULL;
}

RUN_TESTS(all_tests);

