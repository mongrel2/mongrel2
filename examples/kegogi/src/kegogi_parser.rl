#include <stdio.h>
#include <stdlib.h>

#include "kegogi_parser.h"

#include <bstring.h>
#include "kegogi.h"
#include "httpclient.h"
#include <dbg.h>

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

    empty_line = ws* comment? '\n';
    command = (send_command empty_line* expect_command) >clear @command;

  
    main := (command | empty_line)*;
}%%


%% write data;

int parse_kegogi_file(const char *path, Command commands[], int max_commands) 
{
    FILE *script = NULL;
    bstring buffer = NULL;
    int idx = 0;
    bstring host = NULL;
    bstring port = NULL;
    bstring uri = NULL;
    bstring method = NULL;
    bstring status_code = NULL;
    char *mark = NULL;
    int cs = 0;
    char *p = NULL;
    char *pe = NULL;

    script = fopen(path, "r");
    check(script, "Failed to open file: %s", path);

    buffer = bread((bNread)fread, script);
    check_mem(buffer);

    bstring default_host = bfromcstr("localhost");
    bstring default_port = bfromcstr("80");

    p = bdata(buffer);
    pe = p + blength(buffer);

    %% write init;
    %% write exec;

error:
    //fallthrough on purpose
    bdestroy(default_host);
    bdestroy(default_port);
    bdestroy(buffer);
    if(script) fclose(script);

    return idx;
}
