#include "config_file.h"
#include "cli.h"
#include "commands.h"
#include <dbg.h>
#include <stdio.h>

static inline bstring option(Command *cmd, const char *name, const char *def)
{
    hnode_t *val = hash_lookup(cmd->options, name);

    if(def != NULL) {
        return val == NULL ? bfromcstr(def) : hnode_get(val);
    } else {
        return hnode_get(val);
    }
}

int Command_load(Command *cmd)
{
    bstring db_file = option(cmd, "db", "config.sqlite");
    bstring conf_file = option(cmd, "config", "mongrel2.conf");

    debug("LOADING db: %s, config: %s", bdata(db_file), bdata(conf_file));

    Config_load(bdata(conf_file), bdata(db_file));

    return 0;

error:
    return -1;
}

int Command_help(Command *cmd)
{
    printf("Mongrel2 m2sh has these commands available:\n");
    printf("\tload config help\n");
    printf("Sorry there's not much better help yet.\n");

    return 0;
}


typedef int (*Command_handler_cb)(Command *cmd);

typedef struct CommandHandler {
    const char *name;
    Command_handler_cb cb;
} CommandHandler;


static CommandHandler COMMAND_MAPPING[] = {
    {.name = "load", .cb = Command_load},
    {.name = "config", .cb = Command_load},
    {.name = "help", .cb = Command_help},
    {.name = NULL, .cb = NULL}
};

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
