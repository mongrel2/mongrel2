#include <stdio.h>
#include <stdlib.h>

#include "kegogi_parser.h"

#include <bstr/bstrlib.h>
#include "kegogi.h"
#include "httpclient.h"
#include "util.h"

%%{
    machine kegogi;
    action handle_command {
        if(idx < max_commands && uri != NULL && method != NULL) {
            host = host ? host : bstrcpy(default_host);
	    Request *req = Request_create(host, port, method, uri);
	    commands[idx++].request = req;
        }
	else
	    printf("error\n");
    }

    action clear {
        method = NULL;
 	port = default_port;
	host = NULL;
	uri = NULL;
    }

    action mark {
        mark = fpc;
    }

    action method {
    	if(idx < max_commands) {
	    method = blk2bstr(mark, fpc - mark);
  	    bconchar(method, '\0');
        }
    }

    action host {
    	if(idx < max_commands) {
            host = blk2bstr(mark, fpc - mark);
            bconchar(host, '\0');
        }
    }

    action port {
        port = atoin(mark, fpc - mark);
    }

    action uri {
    	if(idx < max_commands) {
            uri = blk2bstr(mark, fpc - mark);
	    bconchar(uri, '\0');
        }
    }

    ws = (space - '\n')+;

    method = (upper | digit){1,20} >mark %method;

    protocol = alpha+ '://';
    host = ([^:/]+) >mark %host;
    port = ':' (digit+) >mark %port;
    uri = ('/' (^space)*) >mark %uri;

    url = ((protocol? host port?)? uri);
    
    comment = '#' [^\n]*;
    command = (ws? method ws url ws? '\n') >clear @handle_command;
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

    bstring host, uri, method;
    int port;
    char *mark, *p = buffer, *pe = buffer + size, *eof = pe;
    int cs;

    %% write init;
    %% write exec;

    return idx;
}
