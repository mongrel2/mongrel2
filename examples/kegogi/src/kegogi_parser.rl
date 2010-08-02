#include <stdio.h>
#include <stdlib.h>

#include "kegogi_parser.h"

#include <bstring.h>
#include "kegogi.h"
#include "httpclient.h"
#include "util.h"

%%{
    machine kegogi;
    action command {
        if(idx < max_commands && uri != NULL && method != NULL) {
            host = host ? host : bstrcpy(default_host);
	    int numPort = port ? atoi(bdata(port)) : default_port;
	    if(port) bdestroy(port);
	    Request *req = Request_create(host, numPort, method, uri);
	    commands[idx++].request = req;
        }
	else
	    printf("error\n");
    }

    action clear {
        method = NULL;
 	port = NULL;
	host = NULL;
	uri = NULL;
    }

    action mark {
        mark = fpc;
    }

    action method {
        method = blk2bstr(mark, fpc - mark);
    }

    action host {
        host = blk2bstr(mark, fpc - mark);
    }

    action port {
        port = blk2bstr(mark, fpc - mark);
    }

    action uri {
        uri = blk2bstr(mark, fpc - mark);
    }

    action status_code {
        status_code = atoin(mark, fpc - mark);
    }        

    action param_name {
        param_name = blk2bstr(mark, fpc - mark);
    }        

    action dictionary {

    }

    action pattern {
        pattern = blk2bstr(mark, fpc - mark);
    }


    ws = (space - '\n')+;

    method = (upper | digit){1,20} >mark %method;

    protocol = alpha+ '://';
    host = ([^:/]+) >mark %host;
    port = ':' (digit+) >mark %port;
    uri = ('/' (^space)*) >mark %uri;

    url = ((protocol? host port?)? uri);
    
    comment = '#' [^\n]*;

    command = (ws? method ws url ws? comment? '\n') >clear @command;
    empty_line = ws? comment? '\n';
  
    main := (command | empty_line)*;
}%%


%% write data;

int parse_kegogi_file(const char *path, Command commands[], int max_commands) {
    FILE *f = fopen(path, "r");
    fseek(f, 0L, SEEK_END);
    int size = ftell(f);
    fseek(f, 0L, SEEK_SET);
    char *buffer = malloc(size);
    fread(buffer, 1, size, f);

    int idx = 0;

    bstring default_host = bfromcstr("localhost");
    int default_port = 80;

    bstring host, port, uri, method;
    char *mark, *p = buffer, *pe = buffer + size, *eof = pe;
    int cs;

    %% write init;
    %% write exec;

    return idx;
}
