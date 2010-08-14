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

static int verify_response(Expect *expected, Response *actual);

void print_indent(int indent_level) {
    int i = 0;
    for(i = 0; i < indent_level; i++) printf("  ");
}

void print_param(int indent_level, Param *p) {
    print_indent(indent_level);
    printf("%s = ", p->name->data);
    if(p->type == STRING)
        printf("\"%s\"", p->data.string->data);
    else if(p->type == PATTERN)
        printf("(%s)", p->data.pattern->data);
    else if(p->type == DICT) {
        printf("{\n");
        Param *p2;
        dnode_t *d;
        ParamDict_foreach(p->data.dict, p2, d) {
            print_param(indent_level + 1, p2);
        }
        print_indent(indent_level);
        printf("}");
    }
    printf("\n");
}

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
        
    int nCommands = parse_kegogi_file(bdata(path), &commandList);
    debug("nCommands = %d", nCommands);

    int i;
    for(i = 0; i < nCommands; i++) {
        Request *req = Request_create(bstrcpy(commands[i].send.host),
                                      atoi(bdata(commands[i].send.port)),
                                      bstrcpy(commands[i].send.method),
                                      bstrcpy(commands[i].send.uri));
        Response *actual = Response_fetch(req);
        
        debug("send %s %s:%s%s",
              bdata(commands[i].send.method), 
              bdata(commands[i].send.host),
              bdata(commands[i].send.port),
              bdata(commands[i].send.uri));
        Param *p;
        dnode_t *d;
        ParamDict_foreach(commands[i].send.params, p, d) {
            print_param(1, p);
        }

        debug("expect %s",
              bdata(commands[i].expect.status_code));
        ParamDict_foreach(commands[i].expect.params, p, d) {
            print_param(1, p);
        }

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
