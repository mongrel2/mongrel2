#include "config_file.h"
#include "cli.h"
#include "commands.h"
#include <dbg.h>
#include <stdio.h>
#include "linenoise.h"
#include <stdlib.h>

typedef int (*Command_handler_cb)(Command *cmd);

typedef struct CommandHandler {
    const char *name;
    const char *help;
    Command_handler_cb cb;
} CommandHandler;


static inline bstring option(Command *cmd, const char *name, const char *def)
{
    hnode_t *val = hash_lookup(cmd->options, name);

    if(def != NULL) {
        return val == NULL ? bfromcstr(def) : hnode_get(val);
    } else {
        return hnode_get(val);
    }
}

static int Command_load(Command *cmd)
{
    bstring db_file = option(cmd, "db", "config.sqlite");
    bstring conf_file = option(cmd, "config", "mongrel2.conf");

    debug("LOADING db: %s, config: %s", bdata(db_file), bdata(conf_file));

    Config_load(bdata(conf_file), bdata(db_file));

    return 0;

error:
    return -1;
}


static int Command_shell(Command *cmd)
{
    char *line = NULL;
    bstring args = NULL;
    char *home_dir = getenv("HOME");
    bstring hist_file = NULL;

    if(home_dir != NULL) {
        hist_file = bformat("%s/.m2sh", home_dir);
        linenoiseHistoryLoad(bdata(hist_file));
    } else {
        log_err("You don't have a HOME environment variable. Oh well, no history.");
        hist_file = NULL;
    }

    while((line = linenoise("m2> ")) != NULL) {
        if (line[0] != '\0') {
            args = bformat("%s %s", bdata(cmd->name), line);
            Command_run(args);
            bdestroy(args);

            if(hist_file) {
                linenoiseHistoryAdd(line);
                linenoiseHistorySave(bdata(hist_file)); /* Save every new entry */
            }
        }

        free(line);
    }

    return 0;

error:
    return -1;
}

static int Command_dump(Command *cmd)
{

    return -1;
}

static int Command_servers(Command *cmd)
{

    return -1;
}


static int Command_hosts(Command *cmd)
{
    return -1;
}


static int Command_init(Command *cmd)
{
    printf("init is deprecated and simply done for you.\n");
    return 0;
}


static int Command_commit(Command *cmd)
{
    return -1;
}


static int Command_log(Command *cmd)
{
    return -1;
}

static int Command_start(Command *cmd)
{
    return -1;
}

static int Command_stop(Command *cmd)
{
    return -1;
}

static int Command_reload(Command *cmd)
{
    return -1;
}

static int Command_running(Command *cmd)
{
    return -1;
}

static int Command_control(Command *cmd)
{
    return -1;
}

static int Command_version(Command *cmd)
{
    return -1;
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
        .help = "Lists the servers." },
    {.name = "dump", .cb = Command_dump,
        .help = "Dumps a quick view of the db." },
    {.name = "hosts", .cb = Command_hosts,
        .help = "Dumps the hosts a server has." },
    {.name = "init", .cb = Command_init,
        .help = "deprecated, just use load" },
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

int Command_run(bstring arguments)
{
    Command cmd;
    CommandHandler *handler;

    check(cli_params_parse_args(arguments, &cmd) != -1, "Invalid arguments.");

    for(handler = COMMAND_MAPPING; handler->name != NULL; handler++)
    {
        if(biseqcstr(cmd.name, handler->name)) {
            return handler->cb(&cmd);
        }
    }

    log_err("INVALID COMMAND. Use m2sh help to find out what's available.");

error:  // fallthrough on purpose
    return -1;
}
