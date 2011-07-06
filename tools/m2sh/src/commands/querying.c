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


