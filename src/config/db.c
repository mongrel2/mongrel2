/**
 *
 * Copyright (c) 2010, Zed A. Shaw and Mongrel2 Project Contributors.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 * 
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 * 
 *     * Neither the name of the Mongrel2 Project, Zed A. Shaw, nor the names
 *       of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written
 *       permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include "dbg.h"
#include "config/db.h"
#include "assert.h"
#include <stdarg.h>
#include "tnetstrings_impl.h"

sqlite3 *CONFIG_DB = NULL;

int DB_init(const char *path)
{
    check(CONFIG_DB == NULL, "Must close the config db first, tell Zed.");
    return sqlite3_open(path, &CONFIG_DB);
error:
    return -1;
}


void DB_close()
{
    if(CONFIG_DB) {
        sqlite3_close(CONFIG_DB);
        CONFIG_DB = NULL;
    }
}


static inline tns_value_t *DB_convert_column(sqlite3_stmt *stmt)
{
    int col = 0;
    tns_value_t *el = NULL;
    tns_value_t *row = tns_new_list();
    check_mem(row);

    for(col = 0; col < sqlite3_column_count(stmt); col++) {
        switch(sqlite3_column_type(stmt, col)) {
            case SQLITE_INTEGER:
                el = tns_new_integer(sqlite3_column_int(stmt, col));
                break;
            case SQLITE_FLOAT:
                el = tns_new_float(sqlite3_column_double(stmt, col));
                break;
            case SQLITE_TEXT:
                el = tns_parse_string(
                    (const char *)sqlite3_column_text(stmt, col), 
                    sqlite3_column_bytes(stmt, col));
                break;
            case SQLITE_BLOB:
                el = tns_parse_string(
                    sqlite3_column_blob(stmt, col), 
                    sqlite3_column_bytes(stmt, col));
                break;
            case SQLITE_NULL:
                el = tns_get_null();
                break;
            default:
                break;
        }

        check(el != NULL, "Got a NULL for a column");
        tns_add_to_list(row, el);
    }

    return row;

error:
    if(row) tns_value_destroy(row);
    if(el) tns_value_destroy(el);
    return NULL;
}

int DB_valid_schema(tns_value_t *res, int row, int ncols, ...)
{
    va_list argp;
    va_start(argp, ncols);
    int i = 0;

    check(tns_get_type(res) == tns_tag_list, "Invalid result set, must be a list.");
    int rows = darray_end(res->value.list);
    check(rows != -1, "Result isn't in a table format.");
    check(row < rows, "Row is past end of result set: %d > %d", row, rows);

    // get the row they ask for
    tns_value_t *row_data = darray_get(res->value.list, row);

    // make sure it's got the right number of columns
    check(tns_get_type(row_data) == tns_tag_list, "Invalid row %d, must be a list.", row);
    int cols = darray_end(row_data->value.list);
    check(cols == ncols, "Expected %d columns, but result set has %d.", cols, ncols);

    for(i = 0; i < ncols; i++) {
        tns_type_tag expecting = va_arg(argp, tns_type_tag);
        tns_value_t *cell = DB_get(res, row, i);
        check(tns_get_type(cell) == expecting,
                "Row %d, Column %d has wrong type.", row, i);
    }

    va_end(argp);
    return 1;
error:

    va_end(argp);
    return 0;
}

/**
 * DB_exec will run a SQL query, using the sqlite3_mprintf syntax, and
 * convert the results to a tns_value_t* that you can use.  It returns
 * one of four types of results:
 *
 * - tns_tag_null -- The query wasn't expected to return anything, like UPDATE.
 * - tns_tag_list(empty) -- The query expected to return rows, but nothing returned.
 * - tns_tag_list -- There's rows that resulted from the query.
 * - NULL -- There was an error processing your query.
 *
 * You own the row and everything in it, and you should know what the columns
 * are, but they are tagged properly for each type that comes from the query.
 */
tns_value_t *DB_exec(const char *query, ...)
{
    va_list argp;
    va_start(argp, query);
    char *zErrMsg = NULL;
    tns_value_t *res = NULL;
    int rc = 0;
    sqlite3_stmt *stmt = NULL;
    const char *sql_tail = NULL;
    char *sql = NULL;

    check(CONFIG_DB != NULL, "The config database is not open.");

    sql = sqlite3_vmprintf(query, argp);
    check_mem(sql);
  
    rc = sqlite3_prepare_v2(CONFIG_DB, sql, -1, &stmt, &sql_tail);
    check(rc == SQLITE_OK, "SQL error \"%s\" at: '%s'", sqlite3_errmsg(CONFIG_DB), sql_tail);

    // we want to return a tns_tag_null if this is a query with no
    // results, which means we have to do a little bit of weird logic
    rc = sqlite3_step(stmt);

    if(rc == SQLITE_DONE) {
        if(sqlite3_column_count(stmt) == 0) {
            // query like UPDATE with no results possible
            res = tns_get_null();
        } else {
            // query that had results possible but had none
            res = tns_new_list();
        }
    } else if(rc == SQLITE_ROW) {
        // query with results, process them
        res = tns_new_list();

        do {
            tns_value_t *row = DB_convert_column(stmt);
            check(row != NULL, "Failed to convert DB column for sql: '%s'", sql);
            tns_add_to_list(res, row);
        } while((rc = sqlite3_step(stmt)) == SQLITE_ROW);
    }

    check(rc != SQLITE_ERROR, "Failure executing sql: %s", sqlite3_errmsg(CONFIG_DB));
   
    sqlite3_free(sql);
    sqlite3_finalize(stmt);
    va_end(argp);
    return res;

error:
    va_end(argp);
    if(stmt) sqlite3_finalize(stmt);
    if(zErrMsg) sqlite3_free(zErrMsg);
    if(sql) sqlite3_free(sql);
    if(res) tns_value_destroy(res);
    return NULL;
}

/**
 * Gets the tns_value_t at this result's row/col point,
 * or NULL (which will be tns_get_type == tns_tag_invalid)
 * if that row isn't possible.
 */
tns_value_t *DB_get(tns_value_t *res, int row, int col)
{
    check(tns_get_type(res) == tns_tag_list, "Result should be a list.");
    check(row < darray_end(res->value.list),
            "Row %d past end of result set length: %d", row, 
            darray_end(res->value.list));

    tns_value_t *r = darray_get(res->value.list, row);
    check(tns_get_type(r) == tns_tag_list, "Row %d should be a list.", row);
    check(col < darray_end(r->value.list),
            "Column %d past end of result set length: %d", col, 
            darray_end(r->value.list));

    return darray_get(r->value.list, col);
error:
    return NULL;
}

/**
 * Used to iterate through a result set using rows by columns.
 * It returns the number of rows, or -1 if this result set isn't
 * valid (like it's not a table form).  It sets the *cols as an
 * out parameter for the number of cols that each row should have.
 */
int DB_counts(tns_value_t *res, int *cols)
{
    int rows = 0;

    if(tns_get_type(res) == tns_tag_null) {
        *cols = 0;
    } else {
        check(tns_get_type(res) == tns_tag_list, "Result should get a list.");
        rows = darray_end(res->value.list);

        if(rows) {
            tns_value_t *first_row = darray_get(res->value.list, 0);
            check(tns_get_type(first_row) == tns_tag_list,
                    "Wrong column type, should be list.");
            *cols = darray_end(first_row->value.list);
        } else {
            *cols = 0;
        }
    }

    return rows;
error:
    return -1;
}


int DB_lastid()
{
    return (int)sqlite3_last_insert_rowid(CONFIG_DB);
}

