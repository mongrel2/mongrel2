#include "config_file.h"
#include "cli.h"
#include "commands.h"
#include <dbg.h>
#include <stdio.h>
#include "linenoise.h"
#include <stdlib.h>
#include <config/db.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <assert.h>
#include <task/task.h>
#include <register.h>
#include <tnetstrings.h>
#include <tnetstrings_impl.h>
#include <pattern.h>

typedef int (*Command_handler_cb)(Command *cmd);

typedef struct CommandHandler {
    const char *name;
    const char *help;
    Command_handler_cb cb;
} CommandHandler;

#define check_file(S, N, P) check(access((const char *)(S)->data, (P)) == 0, "Can't access %s '%s' properly.", N, bdata((S)))
#define check_no_extra(C) check(list_count((C)->extra) == 0, "Commands only take --option style arguments, you have %d extra.", (int)list_count((C)->extra))

static void print_datum(tns_value_t *col)
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


static inline int log_action(bstring db_file, bstring what, bstring why, bstring where, bstring how)
{
    int rc = 0;
    tns_value_t *res = NULL;
    bstring who = NULL;
    char *user = getlogin() == NULL ? getenv("LOGNAME") : getlogin();
    check(user != NULL, "getlogin failed and no LOGNAME env variable, how'd you do that?");
    who = bfromcstr(user);

    if(access((const char *)db_file->data, R_OK | W_OK) != 0) {
        // don't log if there's no file available
        return 0;
    }
 
    rc = DB_init(bdata(db_file));
    check(rc == 0, "Failed to open db: %s", bdata(db_file));

    if(!where) {
        char name_buf[128] = {0};
        int rc = gethostname(name_buf, 127);
        check(rc == 0, "Failed to get your machines hostname, use -where to force it.");
        where = bfromcstr(name_buf);
    }

    res = DB_exec("INSERT INTO log (who, what, location, how, why) VALUES (%Q, %Q, %Q, %Q, %Q)",
            bdata(who), bdata(what), bdata(where), bdata(how), bdata(why));
    check(res != NULL, "Failed to add log message, you're flying blind.");
    tns_value_destroy(res);

    if(biseqcstr(who, "root")) {
        log_warn("You shouldn't be running things as %s.  Use a safe user instead.", bdata(who));
    }

    bdestroy(who);
    bdestroy(where);
    DB_close();
    return 0;

error:
    if(who) bdestroy(who);
    if(where) bdestroy(where);
    if(res) tns_value_destroy(res);
    DB_close();
    return -1;
}



static inline bstring option(Command *cmd, const char *name, const char *def)
{
    hnode_t *val = hash_lookup(cmd->options, name);

    if(def != NULL) {
        if(val == NULL) {
            // add it so it gets cleaned up later when the hash is destroyed
            log_warn("No option --%s given, using \"%s\" as the default.", name, def);

            bstring result = bfromcstr(def);
            hash_alloc_insert(cmd->options, name, result);
            return result;
        } else {
            return hnode_get(val);
        }
    } else {
        return val == NULL ? NULL : hnode_get(val);
    }
}

static int Command_load(Command *cmd)
{
    bstring db_file = option(cmd, "db", "config.sqlite");
    bstring conf_file = option(cmd, "config", "mongrel2.conf");
    bstring what = NULL;
    bstring why = NULL;

    check_file(conf_file, "config file", R_OK);

    Config_load(bdata(conf_file), bdata(db_file));

    what = bfromcstr("load");
    why = bfromcstr("command");

    log_action(db_file, what, why, NULL, conf_file);

error: // fallthrough
    bdestroy(what);
    bdestroy(why);
    return 0;
}


static inline int linenoise_runner(const char *prompt, int (*callback)(bstring arg, void *data), void *data)
{
    char *line = NULL;
    bstring args = NULL;
    char *home_dir = getenv("HOME");
    bstring hist_file = NULL;

    if(home_dir != NULL) {
        hist_file = bformat("%s/.m2sh", home_dir);
        linenoiseHistoryLoad(bdata(hist_file));
    } else {
        log_warn("You don't have a HOME environment variable. Oh well, no history.");
        hist_file = NULL;
    }

    while((line = linenoise(prompt)) != NULL) {
        if (line[0] != '\0') {
            args = bformat("%s", line);
            callback(args, data);
            bdestroy(args);

            if(hist_file) {
                linenoiseHistoryAdd(line);
                linenoiseHistorySave(bdata(hist_file)); /* Save every new entry */
            }
        }

        free(line);
    }

    bdestroy(hist_file);
    return 0;
}


static int run_command(bstring line, void *ignored)
{
    bstring args = bformat("m2sh %s", bdata(line));
    int rc = Command_run(args);

    bdestroy(args);
    return rc;
}


static int Command_shell(Command *cmd)
{
    return linenoise_runner("mongrel2> ", run_command, NULL);
}


static inline int simple_query_print(bstring db_file, const char *sql)
{
    int rc = 0;

    rc = DB_init(bdata(db_file));
    check(rc != -1, "Failed to open database %s", bdata(db_file));

    tns_value_t *res = DB_exec(sql);
    check(res != NULL, "Query failed: %s.", bdata(db_file));

    int cols = 0;
    int rows = DB_counts(res, &cols);
    check(cols > 0, "Invalid query, it has no columns, which is a bug.");
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

static int Command_servers(Command *cmd)
{
    bstring db_file = option(cmd, "db", "config.sqlite");

    check_file(db_file, "config database", R_OK | W_OK);

    printf("SERVERS:\n------\n");
    return simple_query_print(db_file, "SELECT name, default_host, uuid from server");

error:
    return -1;
}



static int Command_hosts(Command *cmd)
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


static int Command_routes(Command *cmd)
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


static int Command_commit(Command *cmd)
{
    bstring what = option(cmd, "what", "");
    bstring why = option(cmd, "why", NULL);
    bstring where = option(cmd, "where", NULL);
    bstring how = option(cmd, "how", "m2sh");
    bstring db_file = option(cmd, "db", "config.sqlite");

    check_file(db_file, "config database", R_OK | W_OK);

    return log_action(db_file, what, why, where, how);

error:
    return -1;
}


static int Command_log(Command *cmd)
{
    bstring db_file = option(cmd, "db", "config.sqlite");
    check_file(db_file, "config database", R_OK | W_OK);

    printf("LOG MESSAGES:\n------\n");
    return simple_query_print(db_file, "SELECT happened_at, who, location, how, what, why FROM log ORDER BY happened_at");

error:
    return -1;
}

struct ServerRun {
    int ran;
    bstring db_file;
    const char *sudo;
    int murder;
};


static inline int exec_server_operations(Command *cmd,
        int (*callback)(struct ServerRun *, tns_value_t *), const char *select)
{
    int rc = 0;
    tns_value_t *res = NULL;

    bstring db_file = option(cmd, "db", "config.sqlite");
    check_file(db_file, "config database", R_OK | W_OK);

    struct ServerRun run = {.ran = 0, .db_file = db_file, .sudo = "", .murder = 0};

    bstring name = option(cmd, "name", NULL);
    bstring uuid = option(cmd, "uuid", NULL);
    bstring host = option(cmd, "host", NULL);
    bstring sudo = option(cmd, "sudo", NULL);
    bstring every = option(cmd, "every", NULL);
    run.murder = option(cmd, "murder", NULL) != NULL;

    check(!(name && uuid && host), "Just one please, not all of the options.");

    if(sudo) {
        run.sudo = biseqcstr(sudo, "") ? "sudo" : bdata(sudo);
    } else {
        run.sudo = "";
    }

    rc = DB_init(bdata(db_file));
    check(rc == 0, "Failed to open db: %s", bdata(db_file));

    if(every) {
        res = DB_exec("SELECT %s FROM server", select);
    } else if(name) {
        res = DB_exec("SELECT %s FROM server where name = %Q", select, bdata(name));
    } else if(host) {
        res = DB_exec("SELECT %s FROM server where default_host = %Q", select, bdata(host));
    } else if(uuid) {
        // yes, this is necessary, so that the callback runs
        res = DB_exec("SELECT %s FROM server where uuid = %Q", select, bdata(uuid));
    } else {
        sentinel("You must give either -name, -uuid, or -host to start a server.");
    }

    check(tns_get_type(res) == tns_tag_list,
            "Wrong return type from query, should be list.");
    check(callback(&run, res) != -1, "Failed to run internal operation.");

    if(!run.ran) {
        errno = 0;

        if(every) {
            log_err("You specified -every server but didn't load any. Not configured right?");
        } else if(host) {
            log_err("Could not load server with host '%s' from db %s.", bdata(host), bdata(db_file));
        } else if(uuid) {
            log_err("Could not load server with uuid '%s' from db %s.", bdata(uuid), bdata(db_file));
        } else if(name) {
            log_err("Could not load server named '%s' from db %s.", bdata(name), bdata(db_file));
        } else {
            log_err("Well looks like you broke something, please report what you did to mongrel2.org.");
        }

        sentinel("Error loading the requested server, see above for why.");
    }

    if(res) tns_value_destroy(res);
    DB_close();
    return 0;

error:
    if(res) tns_value_destroy(res);
    DB_close();
    return -1;
}


static int run_server(struct ServerRun *r, tns_value_t *res)
{
    r->ran = 0;
    int cols = 0;
    int rows = DB_counts(res, &cols);
    check(rows == 1 && cols == 1, "Failed to find requested record.");
    tns_value_t *uuid_val = DB_get(res, 0, 0);

    check(uuid_val != NULL && tns_get_type(uuid_val) == tns_tag_string,
            "Invalid value for the server uuid on run.");

    bstring command = bformat("%s mongrel2 %s %s", r->sudo,
            bdata(r->db_file), 
            bdata(uuid_val->value.string));

    system(bdata(command));

    bdestroy(command);

    r->ran = 1;
    return 0;

error:
    return -1;
}

static int Command_start(Command *cmd)
{
    return exec_server_operations(cmd, run_server, "uuid");
}

bstring read_pid_file(bstring pid_path)
{
    FILE *pid_file = fopen(bdata(pid_path), "r");
    bstring pid = NULL;

    if(pid_file == NULL) {
        return NULL;
    } else {
        pid = bread((bNread)fread, pid_file);
        fclose(pid_file); pid_file = NULL;
    }

    return pid;
}

static int locate_pid_file(tns_value_t *res)
{
    bstring pid = NULL;
    bstring pid_path = NULL;

    int cols = 0;
    int rows = DB_counts(res, &cols);
    check(rows == 1 && cols == 2, "Wrong number of results.");

    tns_value_t *chroot = DB_get(res, 0, 0);
    check(tns_get_type(chroot) == tns_tag_string, "Wrong result for server chroot, should be a string.");

    tns_value_t *pid_file = DB_get(res, 0, 1);
    check(tns_get_type(pid_file) == tns_tag_string, "Wrong result for server pid_file, should be a string.");

    pid_path = bformat("%s%s", bdata(chroot->value.string), bdata(pid_file->value.string));

    pid = read_pid_file(pid_path);
    check(pid, "Couldn't read the PID from %s", bdata(pid_path));

    int result = atoi((const char *)pid->data);

    bdestroy(pid);
    bdestroy(pid_path);
    return result;

error:
    bdestroy(pid);
    bdestroy(pid_path);
    return -1;
}

static int kill_server(struct ServerRun *r, tns_value_t *res, int signal)
{
    int pid = locate_pid_file(res);
    check(pid != -1, "Failed to read the pid_file.");

    int rc = kill(pid, signal);
    check(rc == 0, "Failed to stop server with PID: %d", pid);

    r->ran = 1;
    return 0;

error:
    r->ran = 0;
    return -1;
}

static int stop_server(struct ServerRun *r, tns_value_t *res)
{
    int signal = r->murder ? SIGTERM : SIGINT;
    return kill_server(r, res, signal);
}


static int Command_stop(Command *cmd)
{
    return exec_server_operations(cmd, stop_server, "chroot, pid_file");
}

static int reload_server(struct ServerRun *r, tns_value_t *res)
{
    return kill_server(r, res, SIGHUP);
}


static int Command_reload(Command *cmd)
{
    return exec_server_operations(cmd, reload_server, "chroot, pid_file");
}

static int check_server(struct ServerRun *r, tns_value_t *res)
{
    int rc = 0;
    int pid = locate_pid_file(res);

    if(pid == -1) {
        printf("mongrel2 is not running because pid_file isn't there.\n");
        r->ran = 1;
        return 0;
    }

    rc = kill(pid, 0);

    if(rc != 0) {
        printf("mongrel2 at PID %d is NOT running.\n", pid);
    } else {
        printf("mongrel2 at PID %d running.\n", pid);
    }

    r->ran = 1;
    return 0;
}

static int Command_running(Command *cmd)
{
    return exec_server_operations(cmd, check_server, "chroot, pid_file");
}


static void bstring_free(void *data, void *hint)
{
    bdestroy((bstring)hint);
}

static struct tagbstring TOKENS = bsStatic(" \t\n=");
static struct tagbstring NUMBER_PATTERN = bsStatic("[\\-0-9]+");

bstring parse_input(bstring inbuf)
{
    size_t len = 0;
    int i = 0;
    char *data = NULL;
    bstring result = NULL;
    tns_value_t *req = tns_new_list();
    tns_value_t *args = tns_new_dict();
    tns_value_t *value = NULL;

    btrimws(inbuf);
    struct bstrList *list = bsplits(inbuf, &TOKENS);
    check((list->qty + 1) % 2 == 0, "USAGE: command arg1=val1 arg2=val2");

    tns_add_to_list(req,
            tns_parse_string(bdata(list->entry[0]), blength(list->entry[0])));

    for(i = 1; i < list->qty; i += 2) {
        bstring key_str = list->entry[i];
        bstring val_str = list->entry[i+1];
        tns_value_t *key = tns_parse_string(bdata(key_str), blength(key_str));

        if(bstring_match(val_str, &NUMBER_PATTERN)) {
            value = tns_parse_integer(bdata(val_str), blength(val_str));
        } else {
            value = tns_parse_string(bdata(val_str), blength(val_str));
        }

        tns_add_to_dict(args, key, value);
    }

    tns_add_to_list(req, args);

    data = tns_render(req, &len);
    check(data != NULL, "Didn't render to a valid TNetstring.");
    check(len > 0, "Didn't create a valid TNetstring.");

    result = blk2bstr(data, len);
    check(result != NULL, "Couldn't convert to string.");

    tns_value_destroy(req);
    bstrListDestroy(list);
    free(data);
    return result;

error:

    if(req) tns_value_destroy(req);
    if(list) bstrListDestroy(list);
    if(data) free(data);
    if(result) bdestroy(result);

    return NULL;
}

struct tagbstring ERROR_KEY = bsStatic("error");
struct tagbstring HEADERS_KEY = bsStatic("headers");
struct tagbstring ROWS_KEY = bsStatic("rows");

static void display_map_style(tns_value_t *headers, tns_value_t *table)
{
    int i = 0;
    int cols = 0;
    int rows = DB_counts(table, &cols);
    check(rows != -1, "Invalid query result, probably not a table.");
    darray_t *names = headers->value.list;

    check(cols == darray_end(names),
            "Server returned a bad result, names isn't same length as elements.");

    if(rows == 1) {
        for(i = 0; i < cols; i++)
        {
            tns_value_t *h = darray_get(names, i);
            tns_value_t *val = DB_get(table, 0, i);
            check(tns_get_type(h) == tns_tag_string, "Headers should be strings.");
            check(tns_get_type(val) != tns_tag_invalid,
                    "Invalid value for column %d of result.", i);

            print_datum(h);
            printf(":  ");
            print_datum(val);
            printf("\n");
        }
    } else {
        sentinel("Asked to display something that's not in map style.");
    }

error: // fallthrough
    return;
}


void display_table_style(tns_value_t *headers, tns_value_t *table)
{
    int col_i = 0;
    int row_i = 0;
    for(col_i = 0; col_i < darray_end(headers->value.list); col_i++)
    {
        tns_value_t *h = darray_get(headers->value.list, col_i);
        check(tns_get_type(h) == tns_tag_string,
                "Headers should be strings, not: %c", tns_get_type(h));
        printf("%s  ", bdata(h->value.string));
    }
    printf("\n");

    int cols = 0;
    int rows = DB_counts(table, &cols);
    check(rows != -1, "Invalid query results, probably not in table format.");

    for(row_i = 0; row_i < rows; row_i++) {
        for(col_i = 0; col_i < cols; col_i++) {
            tns_value_t *col = DB_get(table, row_i, col_i);
            print_datum(col);
            printf("  ");
        }
        printf("\n");
    }
    printf("\n");

error: // fallthrough
    return;
}

void display_response(const char *msg, size_t len)
{
    tns_value_t *resp = tns_parse(msg, len, NULL);
    hnode_t *node = NULL;

    check(tns_get_type(resp) == tns_tag_dict, "Server returned an invalid response, must be a dict.");

    node = hash_lookup(resp->value.dict, &ERROR_KEY);

    if(node) {
        tns_value_t *val = hnode_get(node);
        printf("ERROR: %s\n", bdata(val->value.string));
    } else {
        node = hash_lookup(resp->value.dict, &HEADERS_KEY);
        check(node != NULL, "Server returned an invalid response, need a 'headers'.");
        tns_value_t *headers = hnode_get(node);
        check(tns_get_type(headers) == tns_tag_list, "Headers must be a list, server is screwed up.");

        node = hash_lookup(resp->value.dict, &ROWS_KEY);
        check(node != NULL, "Server returned an invalid response, need a 'rows'.");
        tns_value_t *rows = hnode_get(node);
        check(tns_get_type(rows) == tns_tag_list, "Rows must be a list, server is screwed up.");

        if(darray_end(rows->value.list) == 1) {
            display_map_style(headers, rows);
        } else {
            display_table_style(headers, rows);
        }
    }

error: // fallthrough
    tns_value_destroy(resp);
    return;
}

int send_recv_control(bstring args, void *socket)
{
    int rc = 0;
    zmq_msg_t *outmsg = NULL;
    zmq_msg_t *inmsg = NULL;

    outmsg = calloc(sizeof(zmq_msg_t), 1);
    check_mem(outmsg);
    inmsg = calloc(sizeof(zmq_msg_t), 1);
    check_mem(inmsg);
    
    rc = zmq_msg_init(outmsg);
    check(rc == 0, "Failed to initialize outgoing message.");
    rc = zmq_msg_init(inmsg);
    check(rc == 0, "Failed to initialize incoming message.");

    bstring request = parse_input(args);
    check(request != NULL, "Invalid command, try again.");

    // send the message
    rc = zmq_msg_init_data(outmsg, bdata(request), blength(request)+1, bstring_free, request);
    check(rc == 0, "Failed to init outgoing message.");

    rc = mqsend(socket, outmsg, 0);
    check(rc == 0, "Failed to send message to control port.");
    free(outmsg);

    // recv the response
    rc = mqrecv(socket, inmsg, 0);
    check(rc == 0, "Failed to receive message from control port.");

    display_response((const char *)zmq_msg_data(inmsg), zmq_msg_size(inmsg));

    fflush(stdout);
    free(inmsg);

    return 0;

error:
    if(outmsg) free(outmsg);
    if(inmsg) free(inmsg);
    return -1;
}

int control_server(struct ServerRun *r, tns_value_t *res)
{
    int rc = 0;
    void *socket = NULL;
    bstring prompt = NULL;
    bstring control = NULL;
    r->ran = 1;
    int cols = 0;
    int rows = DB_counts(res, &cols);

    check(rows != -1, "Invalid data given to internal routine control_server.");
    check(rows == 1, "Ambiguous server select, expected 1 but got %d.", rows);

    bstring server_name = DB_get_as(res, 0, 0, string);
    bstring chroot = DB_get_as(res, 0, 1, string);

    check(server_name != NULL && chroot != NULL, 
            "Somehow didn't get a good server_name and chroot.");

    prompt = bformat("m2 [%s]> ", bdata(server_name));
    control = bformat("ipc://%s/run/control", bdata(chroot));
    log_info("Connecting to control port %s", bdata(control));

    mqinit(1);
    Register_init();

    socket = mqsocket(ZMQ_REQ);
    check(socket != NULL, "Failed to create REQ socket.");

    rc = zmq_connect(socket, bdata(control));
    check(rc == 0, "Failed to connect to control port.");

    rc = linenoise_runner(bdata(prompt), send_recv_control, socket);

    bdestroy(prompt);
    bdestroy(control);
    zmq_close(socket);
    zmq_term(ZMQ_CTX);
    return rc;

error:
    if(prompt) bdestroy(prompt);
    if(control) bdestroy(control);
    if(socket) zmq_close(socket);
    return -1;
}

static int Command_control(Command *cmd)
{
    return exec_server_operations(cmd, control_server, "name, chroot");
}

static int Command_version(Command *cmd)
{
#include <version.h>
    printf("%s\n", VERSION);
#undef VERSION

    return 0;
}

static int Command_uuid(Command *cmd)
{
    return system("uuidgen");
}


static int Command_help(Command *cmd);

static CommandHandler COMMAND_MAPPING[] = {
    {.name = "load", .cb = Command_load,
        .help = "Load a config."},
    {.name = "config", .cb = Command_load,
        .help = "Alias for load." },
    {.name = "shell", .cb = Command_shell,
        .help = "Starts an interactive shell." },
    {.name = "servers", .cb = Command_servers,
        .help = "Lists the servers in a config database." },
    {.name = "hosts", .cb = Command_hosts,
        .help = "Lists the hosts in a server." },
    {.name = "routes", .cb = Command_routes,
        .help = "Lists the routes in a host." },
    {.name = "commit", .cb = Command_commit,
        .help = "Adds a message to the log." },
    {.name = "log", .cb = Command_log,
        .help = "Prints the commit log." },
    {.name = "start", .cb = Command_start,
        .help = "Starts a server." },
    {.name = "stop", .cb = Command_stop,
        .help = "Stops a server." },
    {.name = "reload", .cb = Command_reload,
        .help = "Reloads a server." },
    {.name = "running", .cb = Command_running,
        .help = "Tells you what's running." },
    {.name = "control", .cb = Command_control,
        .help = "Connects to the control port." },
    {.name = "version", .cb = Command_version,
        .help = "Prints the Mongrel2 and m2sh version." },
    {.name = "help", .cb = Command_help,
        .help = "Get help, lists commands." },
    {.name = "uuid", .cb = Command_uuid,
        .help = "Prints out a randomly generated UUID." },
    {.name = NULL, .cb = NULL, .help = NULL}
};


static int Command_help(Command *cmd)
{
    CommandHandler *handler;

    lnode_t *ex = list_first(cmd->extra);

    if(ex) {
        bstring arg = lnode_get(ex);

        for(handler = COMMAND_MAPPING; handler->name != NULL; handler++) {
            if(biseqcstr(arg, handler->name)) {
                printf("%s\t%s\n", handler->name, handler->help);
                return 0;
            }
        }

        printf("No help for %s. Use m2sh help to see a list.", bdata(arg));
    } else {
        printf("Mongrel2 m2sh has these commands available:\n\n");
        
        for(handler = COMMAND_MAPPING; handler->name != NULL; handler++) {
            printf("%8s  %s\n", handler->name, handler->help);
        }
    }

    return 0;
}

void Command_destroy(Command *cmd)
{
    int i = 0;

    bdestroy(cmd->progname);
    bdestroy(cmd->name);

    if(cmd->extra) {
        list_destroy_nodes(cmd->extra);
         list_destroy(cmd->extra);
    }

    if(cmd->options) {
        hash_free_nodes(cmd->options);
        hash_destroy(cmd->options);
    }

    // finally, we are really actually done with the tokens
    for(i = 0; i < cmd->token_count; i++) {
        Token_destroy(cmd->tokens[i]);
    }

    free(cmd);
}


int Command_run(bstring arguments)
{
    Command *cmd = calloc(1, sizeof(Command));
    CommandHandler *handler;
    int rc = 0;

    check(cli_params_parse_args(arguments, cmd) != -1, "USAGE: m2sh <command> [options]");
    check_no_extra(cmd);

    for(handler = COMMAND_MAPPING; handler->name != NULL; handler++)
    {
        if(biseqcstr(cmd->name, handler->name)) {
            rc = handler->cb(cmd);
            Command_destroy(cmd);
            return rc;
        }
    }

    log_err("INVALID COMMAND. Use m2sh help to find out what's available.");

error:  // fallthrough on purpose
    Command_destroy(cmd);
    return -1;
}

