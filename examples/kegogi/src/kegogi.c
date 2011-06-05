#include "kegogi.h"
#include "fuzzrnd.h"
#include <http11/httpclient_parser.h>
#include <dbg.h>
#include <task/task.h>
#include <bstr/bstrlib.h>
#include <pattern.h>
#include <getopt.h>

#include "httpclient.h"
#include "kegogi_parser.h"
#include "kegogi_tokens.h"

#define MAX_COMMANDS 1024

struct tagbstring DEFAULT_HOST_KEY = bsStatic("host");
struct tagbstring DEFAULT_PORT_KEY = bsStatic("port");

static int verify_response(Expect *expect, Request *req, Response *actual);
static Request *create_request(Send *send, ParamDict *defaults);

typedef struct RunKegogiArgs {
    bstring script_path;
    int num_threads;
    int iterations_per_thread;
} RunKegogiArgs;

typedef struct KegogiTestArgs {
    int num_commands;
    Command *commands;
    ParamDict *defaults;
    int tests_per_thread;
    Rendez *rendez;
    int threads_running;
    int passed;
    int failed;
} KegogiTestArgs;

void kegogitest(void *arg)
{
    KegogiTestArgs *args = (KegogiTestArgs*) arg;

    int i;
    for(i = 0; i < args->tests_per_thread; i++) {
        int n = i % args->num_commands;
        Request *req = create_request(&args->commands[n].send, args->defaults);

        Response *actual = Response_fetch(req);
        
        if(verify_response(&args->commands[n].expect, req, actual))
            args->passed++;
        else
            args->failed++;
        Request_destroy(req);
        Response_destroy(actual);
    }

    args->threads_running--;
    taskwakeup(args->rendez);

    taskexit(0);
 }

void runkegogi(void *arg)
{
    RunKegogiArgs *runArgs = (RunKegogiArgs *) arg;

    KegogiTestArgs *testArgs = calloc(sizeof(*testArgs), 1);
    check_mem(testArgs);

    testArgs->passed = 0;
    testArgs->failed = 0;

    // calloc zeroes out the rendez, which is all that's required to initialize
    // it (per the docs)
    testArgs->rendez = calloc(sizeof(*testArgs->rendez), 1);

    testArgs->threads_running = runArgs->num_threads;
    testArgs->commands = calloc(sizeof(Command), MAX_COMMANDS);
    check_mem(testArgs->commands);

    testArgs->num_commands = parse_kegogi_file(bdata(runArgs->script_path), 
                                               testArgs->commands, 
                                               MAX_COMMANDS, 
                                               &testArgs->defaults);
    check(testArgs->num_commands > 0, "No kegogi tests in file");
    testArgs->tests_per_thread = 
        runArgs->iterations_per_thread * testArgs->num_commands;

    int i;
    for(i = 0; i < runArgs->num_threads; i++)
        taskcreate(kegogitest, testArgs, 128 * 1024);

    while(testArgs->threads_running > 0)
        tasksleep(testArgs->rendez);

    printf("Passed %d of %d\n", testArgs->passed,
           testArgs->passed + testArgs->failed);

    free(runArgs);
    free(testArgs);

    taskexitall((testArgs->failed > 0) ? 1 : 0);
    

    free(runArgs);
    taskexit(0);

error:
    if(runArgs) free(runArgs);
    if(testArgs) free(testArgs);

    taskexitall(1);
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
    dbg_set_log(stderr);
    RunKegogiArgs *args = calloc(sizeof(*args), 1);
    check_mem(args);
    args->script_path = NULL;
    args->num_threads = 1;
    args->iterations_per_thread = 1;

    struct option long_options[] = {
        {"threads", 1, 0, 't'},
        {"iterations", 1, 0, 'i'}
    };

    char c;
    while((c = getopt_long(argc, argv, "t:i:", long_options, NULL)) != -1) {
        switch(c) {
        case 't':
            args->num_threads = atoi(optarg);
            check(args->num_threads > 0, "Invalid number of threads");
            break;
        case 'i':
            args->iterations_per_thread = atoi(optarg);
            check(args->iterations_per_thread > 0, "Invalid number of "
                  "iterations per thread");
            break;
        default:
            check(0, "Invalid argument");
        }
    }

    check(optind < argc, "Expected kegogi file");

    args->script_path = bfromcstr(argv[optind]);

    taskcreate(runkegogi, args, 128 * 1024);

    taskexit(0);

error:
    taskexitall(1);
}
