#include <bstring.h>
#include <config/db.h>

void print_datum(tns_value_t *col)
{
    switch(tns_get_type(col)) {
        case tns_tag_string:
            printf("%s", bdata(col->value.string));
            break;
        case tns_tag_number:
            printf("%ld", col->value.number);
            break;
        case tns_tag_float:
            printf("%f", col->value.fpoint);
            break;
        case tns_tag_null:
            printf("(null)");
            break;
        case tns_tag_bool:
            printf("%s", col->value.bool ? "true" : "false");
            break;
        default:
            printf("NA");
    }
}

int simple_query_print(bstring db_file, const char *sql)
{
    int rc = 0;

    rc = DB_init(bdata(db_file));
    check(rc != -1, "Failed to open database %s", bdata(db_file));

    tns_value_t *res = DB_exec(sql);
    check(res != NULL, "Query failed: %s.", bdata(db_file));

    int cols = 0;
    int rows = DB_counts(res, &cols);
    if (rows != 0) check(cols > 0, "Invalid query, it has no columns, which is a bug.");
    check(rows != -1, "Invalid query result, probably not a table.");

    if(rows == 0) {
        log_warn("No results to display.");
    } else {
        int col_i = 0;
        int row_i = 0;
        for(row_i = 0; row_i < rows; row_i ++) {
            for(col_i = 0; col_i < cols; col_i++) {
                print_datum(DB_get(res, row_i, col_i));
                printf(" ");
            }
            printf("\n");
        }
    }

    DB_close();
    return 0;

error:
    DB_close();
    return -1;
}

