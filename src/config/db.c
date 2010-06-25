#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <dbg.h>
#include <config/db.h>
#include <assert.h>

sqlite3 *CONFIG_DB = NULL;


int DB_init(const char *path)
{
    assert(CONFIG_DB == NULL && "Must close it first.");
    return sqlite3_open(path, &CONFIG_DB);
}


void DB_close()
{
    sqlite3_close(CONFIG_DB);
    CONFIG_DB = NULL;
}


int DB_exec(const char *query, 
        int (*callback)(void*,int,char**,char**),
        void *param)
{
    char *zErrMsg = NULL;
    
    int rc = sqlite3_exec(CONFIG_DB, query, callback, param, &zErrMsg);
    check(rc == SQLITE_OK, "Error querying DB: %s", zErrMsg);

    return 0;

error:
    return -1;
}


