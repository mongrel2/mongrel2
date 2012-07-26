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
#include <tnetstrings.h>
#include "../commands.h"

#define darray_get_as(E, I, T) ((tns_value_t *)darray_get((E), (I)))->value.T;

int Command_access_logs(Command *cmd)
{
    bstring log_filename = option(cmd, "log", "logs/access.log");
    check(log_filename, "Invalid log file given.");

    FILE *log_file = fopen(bdata(log_filename), "r");
    check(log_file != NULL, "Failed to open log file: %s", bdata(log_filename));

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

error:
    return -1;
}

