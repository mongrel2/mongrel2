#include "kegogi.h"
#include "fuzzrnd.h"
#include <http11/httpclient_parser.h>
#include <dbg.h>
#include <task/task.h>
#include <bstr/bstrlib.h>
#include <pattern.h>

#include "httpclient.h"
#include "kegogi_parser.h"
#include "kegogi_tokens.h"

FILE *LOG_FILE = NULL;
#define MAX_COMMANDS 1024

struct tagbstring DEFAULT_HOST_KEY = bsStatic("host");
struct tagbstring DEFAULT_PORT_KEY = bsStatic("port");

static int verify_response(Expect *expect, Request *req, Response *actual);
static Request *create_request(Send *send, ParamDict *defaults);

void runkegogi(void *arg)
{
    bstring path = (bstring) arg;
    Command commands[MAX_COMMANDS];
    ParamDict *defaults;
    int passed = 0;
    int failed = 0;
    int num_commands = 0;

    num_commands = parse_kegogi_file(bdata(path), commands, MAX_COMMANDS,
                                     &defaults);

    int i;
    for(i = 0; i < num_commands; i++) {
        Request *req = create_request(&commands[i].send, defaults);

        Response *actual = Response_fetch(req);
        
        if(verify_response(&commands[i].expect, req, actual))
            passed++;
        else
            failed++;
        Request_destroy(req);
        Response_destroy(actual);
    }
    
    printf("Passed: %d / %d\n", passed, num_commands);

    taskexitall(failed == 0 ? 0 : 1);
}

static Request *create_request(Send *send, ParamDict *defaults)
{
    check(send != NULL, "create_request cannot accept NULL send arg");
    struct tagbstring default_host_key = bsStatic("host");
    struct tagbstring default_port_key = bsStatic("port");
    Param *p;

    bstring method = send->method;
    check(method != NULL, "Command must specify method");

    bstring uri = send->uri;
    check(uri != NULL, "Command must specify uri");

    bstring host = send->host;
    if(host == NULL) {
        p = ParamDict_get(defaults, &default_host_key);
        if(p && p->type == STRING)
            host = p->data.string;
    }
    check(host != NULL, "Command is missing host and there is no default");

    bstring port = send->port;
    if(port == NULL) {
        p = ParamDict_get(defaults, &default_port_key);
        if(p && p->type == STRING)
            port = p->data.string;
    }
    // No check, if it's NULL, we'll use 80

    return Request_create(bstrcpy(host),
                          (port != NULL) ? atoi(bdata(port)) : DEFAULT_PORT,
                          bstrcpy(method),
                          bstrcpy(uri));

error:
    return NULL;
}

static int verify_response(Expect *expect, Request *req, Response *actual) {
    check(expect != NULL && req != NULL, "command and req must not be NULL");
    struct tagbstring body_key = bsStatic("body");

    if(actual == NULL) {
        printf("Failure when fetching %s:%d%s\n", bdata(req->host), req->port,
               bdata(req->uri));
        printf("  Failed to retrieve response from server, actual is NULL\n");
        return 0;
    }

    int expected_status_code = atoi(bdata(expect->status_code));
    if(expected_status_code != actual->status_code) {
        printf("Failure when fetching %s:%d%s\n", bdata(req->host), req->port,
               bdata(req->uri));
        printf("  Status code was %d, expected %d\n", actual->status_code,
               expected_status_code);
        return 0;
    }

    
    Param *p = ParamDict_get(expect->params, &body_key);
    if(p != NULL) {
        int pass = 0;
        if(p->type == PATTERN)
            pass = (bstring_match(actual->body, p->data.pattern) != NULL);
        else if(p->type == STRING)
            pass = (bstrcmp(actual->body, p->data.string) == 0);

        if(!pass) {
            printf("Failure when fetching %s:%d%s\n", bdata(req->host),
                   req->port, bdata(req->uri));
            printf("  Actual: \"%s\"\n", bdata(actual->body));
            if(p->type == PATTERN)
                printf("  Expected: (%s)\n", bdata(p->data.pattern));
            else if (p->type == STRING)
                printf("  Expected: \"%s\"\n", bdata(p->data.string));
            else
                printf("  Expected dict?\n");

            return 0;
        }
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
