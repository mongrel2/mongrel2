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

void taskmain(int argc, char *argv[])
{
    LOG_FILE = stderr;
    check(argc > 1, "Expected kegogi file");

    Command commands[MAX_COMMANDS];
    int nCommands = parse_kegogi_file(argv[1], commands, MAX_COMMANDS);

    int i;
    for(i = 0; i < nCommands; i++) {
        Request *req = commands[i].request;
        Response *rsp = Response_fetch(req);

        if(rsp == NULL)
            debug("Failure to fetch %s:%d%s", bdata(req->host), req->port,
                  bdata(req->uri));
        else
            debug("Fetched %s:%d%s with status code %d", bdata(req->host), 
                   req->port, bdata(req->uri), rsp->status_code);
        Request_destroy(req);
        Response_destroy(rsp);
                        
    }

    taskexit(0);
error:

    taskexitall(1);
}
