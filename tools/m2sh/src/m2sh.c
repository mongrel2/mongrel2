#include <dbg.h>
#include <task/task.h>
#include <bstring.h>
#include <pattern.h>
#include "config_file.h"
#include "ast.h"
#include <config/db.h>
#include "constants.h"

FILE *LOG_FILE = NULL;

struct tagbstring SERVER_SQL = bsStatic("INSERT INTO server (uuid, access_log, error_log, pid_file, chroot, default_host, name, port) VALUES (%Q, %Q, %Q, %Q, %Q, %Q, %Q, %s);");

struct tagbstring HOST_SQL = bsStatic("INSERT INTO host (server_id, name, matching) VALUES (%d, %Q, %Q);");

struct tagbstring SETTING_SQL = bsStatic("INSERT INTO setting (key, value) VALUES (%Q, %Q);");

Value *AST_get(hash_t *fr, const char *name, ValueType type)
{
    hnode_t *hn = hash_lookup(fr, name);
    check(hn, "Failed to find %s", name);

    Value *val = hnode_get(hn);
    check(val->type == type, "Invalid type for %s, should be %s not %s",
            name, Value_type_name(type), Value_type_name(val->type));
    return val;

error:
    return NULL;
}

bstring AST_get_bstr(hash_t *fr, const char *name, ValueType type)
{
    Value *val = AST_get(fr, name, type);
    check(val != NULL, "Failed resolving %s %s.", name, Value_type_name(type));

    return val->as.string->data;

error:
    return NULL;
}

#define AST_str(H, N, T) bdata(AST_get_bstr(H, N, T))


#define CONFIRM_TYPE(N) check(Value_is(val, CLASS), "Not a class.");\
    check(biseqcstr(Class_ident(val->as.cls), N), "Should be a " # N ".");

int SERVER_ID = 0;
int HOST_ID = 0;

int Dir_load(hash_t *settings, Value *val)
{
    CONFIRM_TYPE("Dir");

error:
    return -1;
}

int Handler_load(hash_t *settings, Value *val)
{
    CONFIRM_TYPE("Handler");


error:
    return -1;
}


int Proxy_load(hash_t *settings, Value *val)
{
    CONFIRM_TYPE("Proxy");

error:
    return -1;
}


void Settings_load(hash_t *settings, const char *name, Value *val)
{
    hash_t *sets = val->as.hash;

    const char *sql = sqlite3_mprintf(bdata(&SETTING_SQL),
            name, bdata(val->as.string));

    int rc = DB_exec(sql, NULL, NULL);
    check(rc == 0, "Failed to add setting: %s=%s",
            name, bdata(val->as.string));

    return;
error:
    log_err("Settings failure.");
}


void Route_load(hash_t *settings, const char *name, Value *val)
{
    debug("ROUTE KEY: %s, type: %s", name, Value_type_name(val->type));
}

void Host_load(hash_t *settings, Value *val)
{
    CONFIRM_TYPE("Host");
    Class *cls = val->as.cls;

    const char *name = AST_str(cls->params, "name", VAL_QSTRING);
    check(name, "No name set for Host.");

    const char *sql = sqlite3_mprintf(bdata(&HOST_SQL), SERVER_ID, name, name);
    
    int rc = DB_exec(sql, NULL, NULL);
    check(rc == 0, "Failed to store Host: %s", name);

    HOST_ID = DB_lastid();

    Value *routes = AST_get(cls->params, "routes", VAL_HASH);
    check(routes, "Didn't find any routes for %s", name);

    AST_walk_hash(settings, routes, Route_load);

    return;

error:
    log_err("Failed loading host.");
}


void Server_load(hash_t *settings, Value *val)
{
    CONFIRM_TYPE("Server");

    Class *cls = val->as.cls;

    DB_init("test.sqlite");

    int rc = DB_exec(bdata(&CONFIG_SCHEMA), NULL, NULL);

    const char *sql = sqlite3_mprintf(bdata(&SERVER_SQL),
            AST_str(cls->params, "uuid", VAL_QSTRING),
            AST_str(cls->params, "access_log", VAL_QSTRING),
            AST_str(cls->params, "error_log", VAL_QSTRING),
            AST_str(cls->params, "pid_file", VAL_QSTRING),
            AST_str(cls->params, "chroot", VAL_QSTRING),
            AST_str(cls->params, "default_host", VAL_QSTRING),
            AST_str(cls->params, "name", VAL_QSTRING),
            AST_str(cls->params, "port", VAL_NUMBER));

    rc = DB_exec(sql, NULL, NULL);
    check(rc == 0, "Failed to exec SQL: %s", sql);

    SERVER_ID = DB_lastid();

    Value *hosts = AST_get(cls->params, "hosts", VAL_LIST);
    check(hosts != NULL, "Could not find Server.hosts setting in host %s:%s", 
            AST_str(cls->params, "uuid", VAL_QSTRING),
            AST_str(cls->params, "name", VAL_QSTRING));

    AST_walk_list(settings, hosts->as.list, Host_load);

    Value *set = AST_get(settings, "settings", VAL_HASH);

    if(set) {
        AST_walk_hash(settings, set, Settings_load);
    }

    return;

error:
    log_err("Configuration error, aborting.");
}


void taskmain(int argc, char *argv[])
{
    LOG_FILE = stderr;
    if(argc != 2) {
        debug("Doesn't do much yet, give it the path to a config file.");
        taskexitall(1);
    } else {
        hash_t *settings = Parse_config_file(argv[1]);
        check(settings != NULL, "Error parsing.");
        AST_walk(settings, Server_load);
        taskexitall(0);
    }

error:
    taskexitall(1);
}
