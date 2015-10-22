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
#include <dbg.h>
#include "cli.h"

#include "commands.h"
#include "commands/running.h"
#include "commands/logging.h"
#include "commands/config.h"
#include "commands/access.h"
#include "commands/querying.h"
#include "commands/helpers.h"

typedef int (*Command_handler_cb)(Command *cmd);

typedef struct CommandHandler {
    const char *name;
    const char *help;
    Command_handler_cb cb;
} CommandHandler;

#define check_no_extra(C) check(list_count((C)->extra) == 0, "Commands only take --option style arguments, you have %d extra.", (int)list_count((C)->extra))


bstring option(Command *cmd, const char *name, const char *def)
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
    {.name = "access", .cb = Command_access_logs,
        .help = "Prints the access log."},
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
    {.name = "route", .cb = Command_route,
        .help = "Tests out routing patterns." },
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

    bstring command_name = option(cmd, "on", NULL);

    if (command_name) {
        for (handler = COMMAND_MAPPING; handler->name != NULL; handler++) {
            if (biseqcstr(command_name, handler->name)) {
                printf("%s\t%s\n", handler->name, handler->help);
                return 0;
            }
        }

        printf("No help for %s. Use m2sh help to see a list.", bdata(command_name));
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

