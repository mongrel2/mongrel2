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
            commands[idx].send.params = sendParams;

            commands[idx].expect.status_code = status_code;
            commands[idx].expect.params = params;

            idx++;
        }
        else
            printf("error\n");

        method = NULL;
        port = NULL;
        host = NULL;
        uri = NULL;
        status_code = NULL;
        params = ParamDict_create();
    }

    action send_command {
        sendParams = params;
        params = ParamDict_create();        
    }

    action clear {
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

    action name { s = blk2bstr(mark, fpc - mark); }
    action pattern { s = blk2bstr(mark, fpc - mark); }
    action string { s = blk2bstr(mark, fpc - mark); }
    action key { key = s; }
    action value { value = s; }
    action param {
        Param *param = Param_create(key, STRING, value);
        ParamDict_set(params, param);
    }
            
    ws = (space - '\n')+;
    comment = '#' [^\n]*;

    string = '\"' (([^\"\\] | ('\\' any))*) >mark %string '\"';
    pattern = '(' (([^(\\] | ('\\' any))*) >mark %pattern ')';
    name = (alpha (alnum | '-')*) >mark %name;
    key = (name | string) %key;
    value = string %value;
    param = (key '=' value ws*) %param;
    method = (upper | digit){1,20} >mark %method;
    protocol = alpha+ '://';
    host = ([^:/]+) >mark %host;
    port = ':' (digit+) >mark %port;
    uri = ('/' (^space)*) >mark %uri;
    url = ((protocol? host port?)? uri);

    send_command = (ws* "send" ws+ method ws+ url (ws+ param*)? comment? '\n') %send_command;

    status_code = (digit+) >mark %status_code;
    expect_command = ws* "expect" ws+ status_code (ws+ param*)? comment? '\n';

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
    bstring s = NULL;
    bstring key = NULL;
    bstring value = NULL;

    char *mark = NULL;
    int cs = 0;
    char *p = NULL;
    char *pe = NULL;
    char *eof = NULL;

    ParamDict *sendParams = NULL;
    ParamDict *expectParams = NULL;
    ParamDict *params = ParamDict_create();

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
    ParamDict_destroy(params);
    if(script) fclose(script);

    return idx;
}
