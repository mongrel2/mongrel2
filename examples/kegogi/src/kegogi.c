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

static int verify_response(Expect *expected, Response *actual);

void print_param(Param *p) {
    Param *p2;
    dnode_t *d;
    
    if(p->type == DICT) {
        printf("    %s = {\n", bdata(p->name));
        ParamDict_foreach(p->data.dict, p2, d) {
            switch(p2->type) {
            case PATTERN:
                printf("        %s: (%s)\n", bdata(p2->name), bdata(p2->data.pattern));
                break;
            case STRING:
                printf("        %s: \"%s\"\n", bdata(p2->name), bdata(p2->data.string));
                break;
            case DICT:
                printf("        %s: {...}\n", bdata(p2->name));
                break;
            }
        }
        printf("    }\n");
    }
    else if(p->type == PATTERN)
        printf("    %s = (%s)\n", bdata(p->name), bdata(p->data.pattern));
    else
        printf("    %s = \"%s\"\n", bdata(p->name), bdata(p->data.string));
}

void runkegogi(void *arg)
{
    bstring path = (bstring) arg;
    Command commands[MAX_COMMANDS];
    int nCommands = parse_kegogi_file(bdata(path), commands, MAX_COMMANDS);
    debug("nCommands = %d", nCommands);
    int i;
    for(i = 0; i < nCommands; i++) {
        Request *req = Request_create(bstrcpy(commands[i].send.host),
                                      atoi((const char *)commands[i].send.port->data),
                                      bstrcpy(commands[i].send.method),
                                      bstrcpy(commands[i].send.uri));
        Response *actual = Response_fetch(req);
        
        debug("send %s %s:%s%s",
              bdata(commands[i].send.method), 
              bdata(commands[i].send.host),
              bdata(commands[i].send.port),
              bdata(commands[i].send.uri));
        debug("expect %s",
              bdata(commands[i].expect.status_code));

        dnode_t *d;
        Param *p;
        printf("send args\n");
        ParamDict_foreach(commands[i].send.params, p, d)
            print_param(p);
        printf("expect args\n");
        ParamDict_foreach(commands[i].expect.params, p, d)
            print_param(p);


        if(actual != NULL)
            debug("actual %d", actual->status_code);
        else
            debug("Response failed");

        if(verify_response(&commands[i].expect, actual))
            debug("Verified = SUCCESS");
        else
            debug("Verified = FAILURE");
        
        Request_destroy(req);
        Response_destroy(actual);
    }

}

static int verify_response(Expect *expected, Response *actual) {
    if(!(expected && actual)) return 0;
    
    return atoi(bdata(expected->status_code)) == actual->status_code;
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
