#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <assert.h>
#include "config.h"

sqlite3 *db = NULL;


int db_open(const char *path)
{
    return sqlite3_open(path, &db);
}


void db_close()
{
    sqlite3_close(db);
}


int db_exec(const char *query, 
        int (*callback)(void*,int,char**,char**),
        void *param)
{
    char *zErrMsg = 0;
    
    int rc = sqlite3_exec(db, query, callback, param, &zErrMsg);

    if(rc != SQLITE_OK) {
        fprintf(stderr, "ERROR QUERYING: %s\n", zErrMsg);
        free(zErrMsg);
        abort();
    }

    return rc;
}


int main(int argc, char **argv)
{
    db_open("config.db");
    
    db_exec("SELECT path, name, style, addr FROM route, handler WHERE route.target = handler.name", config_load_routes, NULL);

    db_exec("SELECT path FROM route", config_print_routes, NULL);

    db_close();
    return 0;
}

