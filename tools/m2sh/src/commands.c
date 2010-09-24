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

typedef int (*Command_handler_cb)(Command *cmd);

typedef struct CommandHandler {
    const char *name;
    const char *help;
    Command_handler_cb cb;
} CommandHandler;

#define check_file(S, N, P) check(access((const char *)(S)->data, (P)) == 0, "Can't access " #N " %s properly.", bdata((S)))
#define check_no_extra(C) check(list_count((C)->extra) == 0, "Commands only take --option style arguments, you have %d extra.", (int)list_count((C)->extra))

static inline int log_action(bstring db_file, bstring what, bstring why, bstring where, bstring how)
{
    int rc = 0;
    char *sql = NULL;
    char *user = getlogin() == NULL ? getenv("LOGNAME") : getlogin();
    check(user != NULL, "getlogin failed and no LOGNAME env variable, how'd you do that?");
    bstring who = bfromcstr(user);

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

    sql = sqlite3_mprintf("INSERT INTO log (who, what, location, how, why) VALUES (%Q, %Q, %Q, %Q, %Q)",
            bdata(who), bdata(what), bdata(where), bdata(how), bdata(why));

    rc = DB_exec(sql, NULL, NULL);
    check(rc == 0, "Failed to add log message, you're flying blind.");


    if(biseqcstr(who, "root")) {
        log_warn("You shouldn't be running things as %s.  Use a safe user instead.", bdata(who));
    }

    bdestroy(who);
    bdestroy(where);
    sqlite3_free(sql);
    DB_close();
    return 0;

error:
    bdestroy(who);
    bdestroy(where);
    sqlite3_free(sql);
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

    check_file(db_file, "config database", R_OK | W_OK);
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


static int table_print(void *param, int cols, char **data, char **names)
{
    int i = 0;

    for(i = 0; i < cols; i++) {
        printf(" %s ", data[i]);
    }

    printf("\n");

    return 0;
}

static inline int simple_query_print(bstring db_file, const char *sql)
{
    int rc = 0;

    rc = DB_init(bdata(db_file));
    check(rc != -1, "Failed to open database %s", bdata(db_file));

    rc = DB_exec(sql, table_print, NULL);
    check(rc == 0, "Query failed: %s.", bdata(db_file));

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
        int (*callback)(void*,int,char**,char**), const char *select)
{
    int rc = 0;
    char *sql = NULL;

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

    if(every) {
        sql= sqlite3_mprintf("SELECT %s FROM server", select);
    } else if(name) {
        sql = sqlite3_mprintf("SELECT %s FROM server where name = %Q", select, bdata(name));
    } else if(host) {
        sql = sqlite3_mprintf("SELECT %s FROM server where default_host = %Q", select, bdata(host));
    } else if(uuid) {
        // yes, this is necessary, so that the callback runs
        sql = sqlite3_mprintf("SELECT %s FROM server where uuid = %Q", select, bdata(uuid));
    } else {
        sentinel("You must give either -name, -uuid, or -host to start a server.");
    }

    rc = DB_init(bdata(db_file));
    check(rc == 0, "Failed to open db: %s", bdata(db_file));

    rc = DB_exec(sql, callback, &run);

    check(run.ran, "Operation on server failed.");

    if(sql) sqlite3_free(sql);
    DB_close();
    return 0;

error:
    if(sql) sqlite3_free(sql);
    DB_close();
    return -1;
}


static int run_server(void *param, int cols, char **data, char **names)
{
    assert(cols == 1 && "Wrong number of colums.");
    struct ServerRun *r = (struct ServerRun *)param;
    r->ran = 0;

    bstring command = bformat("%s mongrel2 %s %s", r->sudo,
            bdata(r->db_file), data[0]);

    system(bdata(command));

    bdestroy(command);

    r->ran = 1;
    return 0;
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


static int stop_server(void *param, int cols, char **data, char **names)
{
    struct ServerRun *r = (struct ServerRun *)param;
    int rc = 0;
    bstring pid_path = bformat("%s%s", data[0], data[1]);

    bstring pid = read_pid_file(pid_path);
    check(pid, "Couldn't read the PID from %s", bdata(pid_path));

    int signal = r->murder ? SIGTERM : SIGINT;

    rc = kill(atoi((const char *)pid->data), signal);
    check(rc == 0, "Failed to stop server with PID: %s", bdata(pid));

    bdestroy(pid);
    bdestroy(pid_path);
    r->ran = 1;
    return 0;

error:
    bdestroy(pid);
    bdestroy(pid_path);
    r->ran = 0;
    return -1;
}

static int Command_stop(Command *cmd)
{
    return exec_server_operations(cmd, stop_server, "chroot, pid_file");
}

static int reload_server(void *param, int cols, char **data, char **names)
{
    struct ServerRun *r = (struct ServerRun *)param;
    int rc = 0;
    bstring pid_path = bformat("%s%s", data[0], data[1]);

    bstring pid = read_pid_file(pid_path);
    check(pid, "Couldn't read the PID from %s", bdata(pid_path));

    rc = kill(atoi((const char *)pid->data), SIGHUP);
    check(rc == 0, "Failed to reload PID: %s", bdata(pid));

    bdestroy(pid);
    bdestroy(pid_path);
    r->ran = 1;
    return 0;

error:
    bdestroy(pid);
    bdestroy(pid_path);
    r->ran = 0;
    return -1;
}


static int Command_reload(Command *cmd)
{
    return exec_server_operations(cmd, reload_server, "chroot, pid_file");
}

static int check_server(void *param, int cols, char **data, char **names)
{
    struct ServerRun *r = (struct ServerRun *)param;
    int rc = 0;
    bstring pid_path = bformat("%s%s", data[0], data[1]);

    bstring pid = read_pid_file(pid_path);

    if(pid == NULL) {
        printf("mongrel2 is not running because pid_file %s isn't there.\n", bdata(pid_path));
        bdestroy(pid_path);
        r->ran = 1;
        return 0;
    }

    rc = kill(atoi((const char *)pid->data), 0);

    if(rc != 0) {
        printf("mongrel2 at PID %s is NOT running.\n", bdata(pid));
    } else {
        printf("mongrel2 at PID %s running.\n", bdata(pid));
    }

    bdestroy(pid);
    bdestroy(pid_path);
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


static int send_recv_control(bstring args, void *socket)
{
    int rc = 0;
    bstring msg_buf = bstrcpy(args);
    check_mem(msg_buf);
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

    // send the message
    rc = zmq_msg_init_data(outmsg, bdata(msg_buf), blength(msg_buf)+1, bstring_free, msg_buf);
    check(rc == 0, "Failed to init outgoing message.");

    rc = mqsend(socket, outmsg, 0);
    check(rc == 0, "Failed to send message to control port.");
    free(outmsg);

    // recv the response
    rc = mqrecv(socket, inmsg, 0);
    check(rc == 0, "Failed to recev message from control port.");

    printf("%.*s\n", (int)zmq_msg_size(inmsg), (const char *)zmq_msg_data(inmsg));
    fflush(stdout);
    free(inmsg);

    return 0;

error:
    if(outmsg) free(outmsg);
    if(inmsg) free(inmsg);
    return -1;
}

static int control_server(void *param, int cols, char **data, char **names)
{
    int rc = 0;
    void *socket = NULL;
    struct ServerRun *r = (struct ServerRun *)param;
    r->ran = 1;
    bstring prompt = bformat("m2 [%s]> ", data[0]);
    bstring control = bformat("ipc://%s/run/control", data[1]);
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
    bdestroy(prompt);
    bdestroy(control);
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
}


int Command_run(bstring arguments)
{
    Command cmd;
    CommandHandler *handler;
    int rc = 0;

    check(cli_params_parse_args(arguments, &cmd) != -1, "USAGE: m2sh <command> [options] (extras) with options being --foo bar or -foo bar.");
    check_no_extra(&cmd);

    for(handler = COMMAND_MAPPING; handler->name != NULL; handler++)
    {
        if(biseqcstr(cmd.name, handler->name)) {
            rc = handler->cb(&cmd);
            Command_destroy(&cmd);
            return rc;
        }
    }

    log_err("INVALID COMMAND. Use m2sh help to find out what's available.");

error:  // fallthrough on purpose
    Command_destroy(&cmd);
    return -1;
}

