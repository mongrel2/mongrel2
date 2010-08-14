#include "kegogi.h"
#include "fuzzrnd.h"
#include "httpclient_parser.h"
#include <dbg.h>
#include <task/task.h>
#include <bstr/bstrlib.h>

#include "httpclient.h"
#include "kegogi_parser.h"
#include "kegogi_tokens.h"

FILE *LOG_FILE = NULL;
#define MAX_COMMANDS 1024

struct tagbstring DEFAULT_HOST_KEY = bsStatic("host");
struct tagbstring DEFAULT_PORT_KEY = bsStatic("port");

static int verify_response(Command *command, Request *req, Response *actual);

void runkegogi(void *arg)
{
    bstring path = (bstring) arg;
    Command commands[MAX_COMMANDS];
    CommandList commandList = {
        .size = MAX_COMMANDS,
        .count = 0,
        .defaults = NULL,
        .commands = commands
    };
    int passed = 0;
    int failed = 0;
    int num_commands = 0;

    bstring default_host = NULL;
    int default_port = 80;

    num_commands = parse_kegogi_file(bdata(path), &commandList);
    if(commandList.defaults != NULL) {
        Param *p;
        p = ParamDict_get(commandList.defaults, &DEFAULT_HOST_KEY);
        if(p && p->type == STRING)
            default_host = p->data.string;
        p = ParamDict_get(commandList.defaults, &DEFAULT_PORT_KEY);
        if(p && p->type == STRING)
            default_port = atoi(bdata(p->data.string));
    }

    int i;
    for(i = 0; i < num_commands; i++) {
        bstring host = commands[i].send.host;
        if(!host) host = default_host;
        check(host != NULL, "No host or default host specified");
        
        int port = default_port;
        if(commands[i].send.port)
            port = atoi(bdata(commands[i].send.port));

        Request *req = Request_create(bstrcpy(host), port,
                                      bstrcpy(commands[i].send.method),
                                      bstrcpy(commands[i].send.uri));
        Response *actual = Response_fetch(req);
        
        if(verify_response(&commands[i], req, actual))
            passed++;
        else
            failed++;
        Request_destroy(req);
        Response_destroy(actual);
    }
    
    printf("Passed: %d / %d\n", passed, num_commands);

    taskexitall(failed == 0 ? 0 : 1);

error:
    debug("Errored out in runkegogi, quitting");
    taskexitall(-1);
}

static int verify_response(Command *command, Request *req, Response *actual) {
    check(command != NULL && req != NULL, "command and req must not be NULL");

    if(actual == NULL) {
        printf("Failure when fetching %s:%d%s\n", bdata(req->host), req->port,
               bdata(req->uri));
        printf("  Failed to retrieve response from server, actual is NULL\n");
        return 0;
    }

    int expected_status_code = atoi(bdata(command->expect.status_code));
    if(expected_status_code != actual->status_code) {
        printf("Failure when fetching %s:%d%s\n", bdata(req->host), req->port,
               bdata(req->uri));
        printf("  Status code was %d, expected %d\n", actual->status_code,
               expected_status_code);
        return 0;
    }

    return 1;

error:
    return 0;
}

void taskmain(int argc, char *argv[])
{
    LOG_FILE = stderr;
    check(argc > 1, "Expected kegogi file");

    taskcreate(runkegogi, bfromcstr(argv[1]), 128 * 1024);

    taskexit(0);

error:
    taskexitall(1);
}
