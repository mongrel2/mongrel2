#include "kegogi_parser.h"

#include <bstring.h>
#include <dbg.h>

#include "param.h"

%%{
    machine foo;

    action mark { m = fpc; }

    action name { s = blk2bstr(m, fpc-m); type = STRING; }
    action string { s = blk2bstr(m, fpc-m); type = STRING; }
    action pattern { s = blk2bstr(m, fpc-m); type = PATTERN; }

    action start_dict { dict = ParamDict_create(); }
    action end_dict { type=DICT; }

    action param_name { param_name = s; s = NULL; }
    action param_value {
        param_type = type;
        param_value = param_type == DICT ? (void*) dict : (void*) s;
        dict = NULL;
        s = NULL;
    }
    action param {
        Param *param = Param_create(param_name, param_type, param_value);
        check_mem(param);
        ParamDict_set(params, param);
        param_value = NULL;
        param_name = NULL;
    }

    action dict_key { dict_key = s; }
    action dict_value { dict_value = s; dict_type = type; }
    action dict_pair { 
        Param *dict_pair = Param_create(dict_key, dict_type, dict_value);
        ParamDict_set(dict, dict_pair);
    }

    action method { method = blk2bstr(m, fpc-m); }

    action host { host = blk2bstr(m, fpc-m); }
    action port { port = blk2bstr(m, fpc-m); }
    action uri { uri = blk2bstr(m, fpc-m); }
    action status_code { status_code = blk2bstr(m, fpc-m); }

    action start_command {
        check(idx < max_commands, "Too many commands");
        params = ParamDict_create();
    }

    action send_command {
        commands[idx].send.method = method;
        commands[idx].send.host = host;
        commands[idx].send.port = port;
        commands[idx].send.uri = uri;
        commands[idx].send.params = params;
        method = host = port = uri = NULL;
        params = NULL;
    }

    action expect_command {
        commands[idx].expect.status_code = status_code;
        commands[idx].expect.params = params;
        status_code = NULL;
        params = NULL;
    }

    action command_pair { idx++; }

    ws = space - '\n';

    name = (alpha (alnum | '-')*) >mark %name;
    pattern = '(' (([^(\\] | ('\\' any))*) >mark %pattern ')';
    string = '\"' (([^\"\\] | ('\\' any))*) >mark %string '\"';
    pair = (ws* (name | string) %dict_key ws* ':' ws* (string | pattern) %dict_value ws*) %dict_pair;
    dict = ('{' (pair ',')* pair? '}') >start_dict %end_dict;

    protocol = alpha+ '://';
    host = [^:/]+ >mark %host;
    port = digit+ >mark %port;
    uri  = ('/' (^ws)*) >mark %uri;
    url  = (protocol? host (':' port)?)? uri;

    method = alpha+ >mark %method;

    status_code = digit+ >mark %status_code;


    param = ((name | string) %param_name ws* '=' ws* (string | pattern | dict) %param_value) %param;

    send_command = ('send' ws+ method ws+ url (ws+ param)*) >start_command %send_command '\n';
    expect_command = ('expect' ws+ status_code (ws+ param)*) >start_command %expect_command '\n';
    command_pair = (send_command expect_command) %command_pair;

    main := command_pair*;
}%%

%% write data;

#include <stdio.h>
#include <string.h>

int parse_kegogi_file(const char *path, Command commands[], int max_commands) {
    FILE *script = NULL;
    bstring buffer = NULL;
    int idx = 0;
    bstring s = NULL;
    ParamType type;
    bstring host = NULL;
    bstring port = NULL;
    bstring uri = NULL;
    bstring method = NULL;
    bstring status_code = NULL;
    bstring param_name = NULL;
    ParamType param_type;
    void* param_value = NULL;
    bstring dict_key = NULL;
    ParamType dict_type;
    void *dict_value = NULL;
    ParamDict *dict = NULL;

    char *m = NULL;
    int cs = 0;
    char *p = NULL;
    char *pe = NULL;
    char *eof = NULL;

    ParamDict *params = NULL;

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

    return idx + 1;
}

