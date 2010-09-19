#include <config/db.h>
#include "constants.h"
#include <bstring.h>
#include "config_file.h"
#include "ast.h"
#include <dbg.h>


#define CONFIRM_TYPE(N) check(Value_is(val, CLASS), "Not a class.");\
    check(biseqcstr(Class_ident(val->as.cls), N), "Should be a " # N ".");

int SERVER_ID = 0;
int HOST_ID = 0;

int Dir_load(hash_t *settings, hash_t *params)
{
    const char *base = AST_str(params, "base", VAL_QSTRING);

    const char *sql = sqlite3_mprintf(bdata(&DIR_SQL),
            base,
            AST_str(params, "index_file", VAL_QSTRING),
            AST_str(params, "default_ctype", VAL_QSTRING));

    int rc = DB_exec(sql, NULL, NULL);
    check(rc == 0, "Failed to load Dir: %s", base);

    return DB_lastid();

error:
    return -1;
}

int Handler_load(hash_t *settings, hash_t *params)
{
    const char *send_spec = AST_str(params, "send_spec", VAL_QSTRING);

    const char *sql = sqlite3_mprintf(bdata(&HANDLER_SQL),
            send_spec,
            AST_str(params, "send_ident", VAL_QSTRING),
            AST_str(params, "recv_spec", VAL_QSTRING),
            AST_str(params, "recv_ident", VAL_QSTRING));

    int rc = DB_exec(sql, NULL, NULL);
    check(rc == 0, "Failed to load Handler: %s", send_spec);

    return DB_lastid();

error:

    return -1;
}


int Proxy_load(hash_t *settings, hash_t *params)
{
    const char *addr = AST_str(params, "addr", VAL_QSTRING);
    const char *port = AST_str(params, "port", VAL_NUMBER);

    const char *sql = sqlite3_mprintf(bdata(&PROXY_SQL),
            addr, port);

    int rc = DB_exec(sql, NULL, NULL);
    check(rc == 0, "Failed to load Proxy: %s:%s", addr, port);
    
    return DB_lastid();

error:
    return -1;
}

int Mimetypes_import()
{
    return DB_exec(bdata(&MIMETYPES_DEFAULT_SQL), NULL, NULL);
}

int Mimetypes_load(hash_t *settings, const char *ext, Value *val)
{
    const char *sql = sqlite3_mprintf(bdata(&MIMETYPE_SQL),
            ext, ext, bdata(val->as.string->data));

    int rc = DB_exec(sql, NULL, NULL);
    check(rc == 0, "Failed to add mimetype: %s=%s", 
            ext, bdata(val->as.string->data));

    return 0;

error:
    return -1;
}

int Settings_load(hash_t *settings, const char *name, Value *val)
{
    const char *sql = sqlite3_mprintf(bdata(&SETTING_SQL),
            name, bdata(val->as.string->data));

    int rc = DB_exec(sql, NULL, NULL);
    check(rc == 0, "Failed to add setting: %s=%s",
            name, bdata(val->as.string->data));

    sqlite3_free(sql);
    return 0;

error:
    sqlite3_free(sql);
    return -1;
}


int Route_load(hash_t *settings, const char *name, Value *val)
{
    check(Value_is(val, CLASS), "Expected a Class but got a %s instead.", Value_type_name(val->type));
    Class *cls = val->as.cls;

    int rc = 0;
    bstring type = bstrcpy(Class_ident(cls));
    btolower(type);

    if(biseqcstr(type, "dir")) {
        rc = Dir_load(settings, cls->params);
    } else if(biseqcstr(type, "proxy")) {
        rc = Proxy_load(settings, cls->params);
    } else if(biseqcstr(type, "handler")) {
        rc = Handler_load(settings, cls->params);
    } else {
        sentinel("Invalid type of route target: %s", bdata(Class_ident(cls)));
    }

    check(rc != -1, "Failed to create target for route %s", name);

    const char *sql = sqlite3_mprintf(bdata(&ROUTE_SQL),
            name, HOST_ID, rc, bdata(type));

    rc = DB_exec(sql, NULL, NULL);
    check(rc == 0, "Failed to intialize route.");

    bdestroy(type);
    return 0;

error:
    bdestroy(type);
    return -1;
}

int Host_load(hash_t *settings, Value *val)
{
    CONFIRM_TYPE("Host");
    Class *cls = val->as.cls;

    const char *name = AST_str(cls->params, "name", VAL_QSTRING);
    check(name, "No name set for Host.");

    const char *sql = sqlite3_mprintf(bdata(&HOST_SQL), SERVER_ID, name, name);
    
    int rc = DB_exec(sql, NULL, NULL);
    check(rc == 0, "Failed to store Host: %s", name);

    HOST_ID = DB_lastid();

    Value *routes = AST_get(settings, cls->params, "routes", VAL_HASH);
    check(routes, "Didn't find any routes for %s", name);

    AST_walk_hash(settings, routes, Route_load);

    return 0;

error:
    return -1;
}


int Server_load(hash_t *settings, Value *val)
{
    CONFIRM_TYPE("Server");
    Class *cls = val->as.cls;
    int rc = 0;

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

    Value *hosts = AST_get(settings, cls->params, "hosts", VAL_LIST);
    check(hosts != NULL, "Could not find Server.hosts setting in host %s:%s", 
            AST_str(cls->params, "uuid", VAL_QSTRING),
            AST_str(cls->params, "name", VAL_QSTRING));

    AST_walk_list(settings, hosts->as.list, Host_load);

    Value *set = AST_get(settings, settings, "settings", VAL_HASH);

    if(set) {
        rc = AST_walk_hash(settings, set, Settings_load);
        check(rc == 0, "Failed to load the settings. Aborting.");
    }

    rc = Mimetypes_import();
    check(rc == 0, "Failed to import default mimetypes.");

    Value *mime = AST_get(settings, settings, "mimetypes", VAL_HASH);
    if(mime) {
        AST_walk_hash(settings, mime, Mimetypes_load);
        check(rc == 0, "Failed to load the mimetypes. Aborting.");
    }

    return 0;

error:
    return -1;
}

static inline int Config_setup(const char *db_file)
{
    DB_init(db_file);
    int rc = DB_exec("begin", NULL, NULL);
    check(rc == 0, "Couldn't start transaction.");

    rc = DB_exec(bdata(&CONFIG_SCHEMA), NULL, NULL);
    check(rc == 0, "Failed to load initial schema.");

    return 0;

error:
    return -1;
}


static inline int Config_commit()
{
    return DB_exec("commit", NULL, NULL);
}


int Config_load(const char *config_file, const char *db_file)
{
    int rc = 0;
    hash_t *settings = Parse_config_file(config_file);

    check(settings != NULL, "Error parsing config file: %s.", config_file);

    rc = Config_setup(db_file);
    check(rc == 0, "Failed to configure config db: %s", db_file);

    rc = AST_walk(settings, Server_load);
    check(rc == 0, "Failed to process the config file: %s", config_file);

    rc = Config_commit();
    check(rc == 0, "Failed to commit config db: %s", db_file);


    DB_close();
    return 0;
error:
    DB_close();
    return -1;
}


