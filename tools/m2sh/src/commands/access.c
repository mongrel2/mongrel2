#include <stdio.h>
#include <tnetstrings.h>
#include "../commands.h"

#define darray_get_as(E, I, T) ((tns_value_t *)darray_get((E), (I)))->value.T;

int Command_access_logs(Command *cmd)
{
    bstring log_filename = option(cmd, "log", "logs/access.log");

    FILE *log_file = fopen(bdata(log_filename), "r");
    int line_number = 0;
    bstring line;

    while ((line = bgets((bNgetc) fgetc, log_file, '\n')) != NULL) {
        line_number++;

        tns_value_t *log_item = tns_parse(bdata(line), blength(line), NULL);

        if (log_item == NULL ||
                tns_get_type(log_item) != tns_tag_list || 
                darray_end(log_item->value.list) < 9
                )
        {
            log_warn("Malformed log line: %d.", line_number);
            continue;
        }

        darray_t *entries = log_item->value.list;

        bstring hostname = darray_get_as(entries, 0, string);
        bstring remote_addr = darray_get_as(entries, 1, string);
        long remote_port = darray_get_as(entries, 2, number);
        long timestamp = darray_get_as(entries, 3, number);
        bstring request_method = darray_get_as(entries, 4, string);
        bstring request_path = darray_get_as(entries, 5, string);
        bstring version = darray_get_as(entries, 6, string);
        long status = darray_get_as(entries, 7, number);
        long size = darray_get_as(entries, 8, number);

        printf("[%ld] %s:%ld %s \"%s %s %s\" %ld %ld\n",
               timestamp,
               bdata(remote_addr),
               remote_port,
               bdata(hostname),
               bdata(request_method),
               bdata(request_path),
               bdata(version),
               status,
               size);

        tns_value_destroy(log_item);
    }

    return 0;
}

