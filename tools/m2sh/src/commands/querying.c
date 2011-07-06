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

#include <unistd.h>
#include <sqlite3.h>
#include <dbg.h>
#include <tnetstrings.h>
#include "../commands.h"
#include "../query_print.h"

int Command_servers(Command *cmd)
{
    bstring db_file = option(cmd, "db", "config.sqlite");

    check_file(db_file, "config database", R_OK | W_OK);

    printf("SERVERS:\n------\n");
    return simple_query_print(db_file, "SELECT name, default_host, uuid from server");

error:
    return -1;
}

int Command_hosts(Command *cmd)
{
    bstring db_file = option(cmd, "db", "config.sqlite");
    bstring server = option(cmd, "server", NULL);
    char *sql = NULL;
    int rc = 0;

    check_file(db_file, "config database", R_OK | W_OK);
    check(server, "You need to give a -server of the server to list hosts from.");

    sql = sqlite3_mprintf("SELECT id, name from host where server_id = (select id from server where name = %Q)", bdata(server));

    printf("HOSTS in %s:\n-----\n", bdata(server));
    rc = simple_query_print(db_file, sql);

error: // fallthrough
    if(sql) sqlite3_free(sql);
    return rc;
}


int Command_routes(Command *cmd)
{
    bstring db_file = option(cmd, "db", "config.sqlite");
    bstring server = option(cmd, "server", NULL);
    bstring host = option(cmd, "host", NULL);
    bstring host_id = option(cmd, "id", NULL);
    char *sql = NULL;
    int rc = 0;

    check_file(db_file, "config database", R_OK | W_OK);

    if(host_id) {
        printf("ROUTES in host id %s\n-----\n", bdata(host_id));
        sql = sqlite3_mprintf("SELECT path from route where host_id=%q", bdata(host_id));
    } else {
        check(server, "Must set the -server name you want or use -id.");
        check(host, "Must set the -host in that server you want or use -id.");

        printf("ROUTES in host %s, server %s\n-----\n", bdata(host), bdata(server));

        sql = sqlite3_mprintf("SELECT route.path from route, host, server where "
                "host.name = %Q and route.host_id = host.id and server.name = %Q and "
                "host.server_id = server.id", bdata(host), bdata(server));
    }

    rc = simple_query_print(db_file, sql);

error: //fallthrough
    if(sql) sqlite3_free(sql);
    return rc;
}


