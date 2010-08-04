#include <stdio.h>
#include <stdlib.h>

#include "kegogi_parser.h"

#include <bstring.h>
#include "kegogi.h"
#include "httpclient.h"

%%{
    machine kegogi;
    action command {
        if(idx < max_commands && uri && method && status_code) {
	    host = host ? host : bstrcpy(default_host);
	    port = port ? port : bstrcpy(default_port);	    

	    commands[idx].send.method = method;
	    commands[idx].send.host = host;
	    commands[idx].send.port = port;
	    commands[idx].send.uri = uri;
	    
	    commands[idx].expect.status_code = status_code;

	    idx++;
        }
	else
	    printf("error\n");
    }

    action clear {
        method = NULL;
 	port = NULL;
	host = NULL;
	uri = NULL;
	status_code = NULL;
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
        status_code = blk2bstr(mark, fpc - mark);
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
    comment = '#' [^\n]*;

    method = (upper | digit){1,20} >mark %method;
    protocol = alpha+ '://';
    host = ([^:/]+) >mark %host;
    port = ':' (digit+) >mark %port;
    uri = ('/' (^space)*) >mark %uri;
    url = ((protocol? host port?)? uri);

    send_command = ws* "send" ws+ method ws+ url ws? comment? '\n';

    status_code = (digit+) >mark %status_code;
    expect_command = ws* "expect" ws+ status_code ws? comment? '\n';

    empty_line = ws? comment? '\n';
    command = (send_command empty_line* expect_command) >clear @command;

  
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
    bstring default_port = bfromcstr("80");

    bstring host, port, uri, method, status_code;
    char *mark, *p = buffer, *pe = buffer + size, *eof = pe;
    int cs;

    %% write init;
    %% write exec;

    bdestroy(default_host);
    bdestroy(default_port);

    return idx;
}
