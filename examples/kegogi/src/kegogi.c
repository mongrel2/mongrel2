#include "kegogi.h"
#include "fuzzrnd.h"
#include "httpclient_parser.h"
#include <dbg.h>
#include <task/task.h>
#include <bstr/bstrlib.h>

#include "httpclient.h"
#include "kegogi_parser.h"

FILE *LOG_FILE = NULL;
#define MAX_COMMANDS 1024

static int verify_response(Response *expected, Response *actual);

void runkegogi(void *arg)
{
    bstring path = (bstring) arg;
    Command commands[MAX_COMMANDS];
    int nCommands = parse_kegogi_file(bdata(path), commands, MAX_COMMANDS);

    int i;
    for(i = 0; i < nCommands; i++) {
        Request *req = commands[i].request;
        Response *expected = commands[i].expected;
        Response *actual = Response_fetch(req);
        
        debug("sending %s to %s:%d%s", bdata(req->method), bdata(req->host), 
              req->port, bdata(req->uri));
        debug("expecting %s", bdata(expected->status_code));
        if(actual != NULL)
            debug("actual %s", bdata(actual->status_code));
        else
            debug("Response failed");

        debug("Verified = %s", verify_response(expected, actual) ? "SUCCESS" : "FAILURE");
        
        Request_destroy(req);
        Response_destroy(actual);
        Response_destroy(expected);
    }

}

static int verify_response(Response *expected, Response *actual) {
    if(!(expected && actual)) return 0;
    
    return biseq(expected->status_code, actual->status_code);
}

void taskmain(int argc, char *argv[])
{
    LOG_FILE = stderr;
    check(argc > 1, "Expected kegogi file");

    taskcreate(runkegogi, bfromcstr(argv[1]), 64 * 1024);

    taskexit(0);

error:
    taskexitall(1);
}
