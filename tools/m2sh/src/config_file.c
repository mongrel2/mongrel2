#include <config/db.h>
#include "constants.h"
#include <bstring.h>
#include "config_file.h"
#include "ast.h"
#include <dbg.h>
#include <stdlib.h>


#define CONFIRM_TYPE(N) check(Value_is(val, CLASS), "Not a class.");\
    check(biseqcstr(Class_ident(val->as.cls), N), "Should be a " # N ".");

int SERVER_ID = 0;
int HOST_ID = 0;

int Dir_load(tst_t *settings, tst_t *params)
{
    const char *base = AST_str(settings, params, "base", VAL_QSTRING);

    char *sql = NULL;
    
    sql = sqlite3_mprintf(bdata(&DIR_SQL),
            base,
            AST_str(settings, params, "index_file", VAL_QSTRING),
            AST_str(settings, params, "default_ctype", VAL_QSTRING));

    int rc = DB_exec(sql, NULL, NULL);
    check(rc == 0, "Failed to load Dir: %s", base);

    sqlite3_free(sql);
    return DB_lastid();

error:
    if(sql) sqlite3_free(sql);
    return -1;
}

int Handler_load(tst_t *settings, tst_t *params)
{
    const char *send_spec = AST_str(settings, params, "send_spec", VAL_QSTRING);
    char *sql = NULL;
    
    sql = sqlite3_mprintf(bdata(&HANDLER_SQL),
            send_spec,
            AST_str(settings, params, "send_ident", VAL_QSTRING),
            AST_str(settings, params, "recv_spec", VAL_QSTRING),
            AST_str(settings, params, "recv_ident", VAL_QSTRING));

    int rc = DB_exec(sql, NULL, NULL);
    check(rc == 0, "Failed to load Handler: %s", send_spec);

    const char *raw_payload = AST_str(settings, params, "raw_payload", VAL_NUMBER);
    if(raw_payload && raw_payload[0] == '1') {
        DB_exec(bdata(&HANDLER_RAW_SQL), NULL, NULL);
    }

    sqlite3_free(sql);
    return DB_lastid();

error:

    if(sql) sqlite3_free(sql);
    return -1;
}


int Proxy_load(tst_t *settings, tst_t *params)
{
    const char *addr = AST_str(settings, params, "addr", VAL_QSTRING);
    const char *port = AST_str(settings, params, "port", VAL_NUMBER);

    char *sql = NULL;
    
    sql = sqlite3_mprintf(bdata(&PROXY_SQL),
            addr, port);

    int rc = DB_exec(sql, NULL, NULL);
    check(rc == 0, "Failed to load Proxy: %s:%s", addr, port);
    
    sqlite3_free(sql);
    return DB_lastid();

error:
    if(sql) sqlite3_free(sql);
    return -1;
}

int Mimetypes_import()
{
    return DB_exec(bdata(&MIMETYPES_DEFAULT_SQL), NULL, NULL);
}

int Mimetypes_load(tst_t *settings, Pair *pair)
{
    const char *ext = bdata(Pair_key(pair));
    Value *val = Pair_value(pair);
    check(val, "Error loading Mimetype %s", bdata(Pair_key(pair)));

    char *sql = NULL;
    
    sql = sqlite3_mprintf(bdata(&MIMETYPE_SQL),
            ext, ext, bdata(val->as.string->data));

    int rc = DB_exec(sql, NULL, NULL);
    check(rc == 0, "Failed to add mimetype: %s=%s", 
            ext, bdata(val->as.string->data));

    sqlite3_free(sql);
    return 0;

error:
    if(sql) sqlite3_free(sql);
    return -1;
}

int Settings_load(tst_t *settings, Pair *pair)
{
    const char *name = bdata(Pair_key(pair));
    Value *val = Pair_value(pair);
    check(val, "Error loading Setting %s", bdata(Pair_key(pair)));

    char *sql = NULL;
    
    sql = sqlite3_mprintf(bdata(&SETTING_SQL),
            name, bdata(val->as.string->data));

    int rc = DB_exec(sql, NULL, NULL);
    check(rc == 0, "Failed to add setting: %s=%s",
            name, bdata(val->as.string->data));

    sqlite3_free(sql);
    return 0;

error:
    if(sql) sqlite3_free(sql);
    return -1;
}


int Route_load(tst_t *settings, Pair *pair)
{
    const char *name = bdata(Pair_key(pair));
    char *sql = NULL;
    Value *val = Pair_value(pair);
    bstring type = NULL;
    int rc = 0;

    check(val, "Error loading route: %s", bdata(Pair_key(pair)));
    check(Value_is(val, CLASS), "Expected a Class but got a %s instead.",
            Value_type_name(val->type));
    Class *cls = val->as.cls;
    type = bstrcpy(Class_ident(cls));
    btolower(type);

    if(cls->id == -1) {
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
        cls->id = rc;
    }

    sql = sqlite3_mprintf(bdata(&ROUTE_SQL), name, HOST_ID, cls->id, bdata(type));

    rc = DB_exec(sql, NULL, NULL);
    check(rc == 0, "Failed to intialize route.");

    sqlite3_free(sql);
    bdestroy(type);
    return 0;

error:
    if(sql) sqlite3_free(sql);
    bdestroy(type);
    return -1;
}

int Host_load(tst_t *settings, Value *val)
{
    CONFIRM_TYPE("Host");
    Class *cls = val->as.cls;
    char *sql = NULL;
    struct tagbstring ROUTES_VAR = bsStatic("routes");

    const char *name = AST_str(settings, cls->params, "name", VAL_QSTRING);
    check(name, "No name set for Host.");

    sql = sqlite3_mprintf(bdata(&HOST_SQL), SERVER_ID, name, name);
    
    int rc = DB_exec(sql, NULL, NULL);
    check(rc == 0, "Failed to store Host: %s", name);

    cls->id = HOST_ID = DB_lastid();

    Value *routes = AST_get(settings, cls->params, &ROUTES_VAR, VAL_HASH);
    check(routes, "Didn't find any routes for %s", name);

    AST_walk_hash(settings, routes, Route_load);

    sqlite3_free(sql);
    return 0;

error:
    if(sql) sqlite3_free(sql);
    return -1;
}


int Server_load(tst_t *settings, Value *val)
{
    CONFIRM_TYPE("Server");
    Class *cls = val->as.cls;
    int rc = 0;
    char *sql = NULL;
    struct tagbstring HOSTS_VAR = bsStatic("hosts");

    sql = sqlite3_mprintf(bdata(&SERVER_SQL),
            AST_str(settings, cls->params, "uuid", VAL_QSTRING),
            AST_str(settings, cls->params, "access_log", VAL_QSTRING),
            AST_str(settings, cls->params, "error_log", VAL_QSTRING),
            AST_str(settings, cls->params, "pid_file", VAL_QSTRING),
            AST_str(settings, cls->params, "chroot", VAL_QSTRING),
            AST_str(settings, cls->params, "default_host", VAL_QSTRING),
            AST_str(settings, cls->params, "name", VAL_QSTRING),
            AST_str(settings, cls->params, "port", VAL_NUMBER));

    rc = DB_exec(sql, NULL, NULL);
    check(rc == 0, "Failed to exec SQL: %s", sql);

    cls->id = SERVER_ID = DB_lastid();

    Value *hosts = AST_get(settings, cls->params, &HOSTS_VAR, VAL_LIST);
    check(hosts != NULL, "Could not find Server.hosts setting in host %s:%s", 
            AST_str(settings, cls->params, "uuid", VAL_QSTRING),
            AST_str(settings, cls->params, "name", VAL_QSTRING));

    AST_walk_list(settings, hosts->as.list, Host_load);


    sqlite3_free(sql);
    return 0;

error:
    if(sql) sqlite3_free(sql);
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
    tst_t *settings = NULL;
    struct tagbstring SETTINGS_VAR = bsStatic("settings");
    struct tagbstring MIMETYPES_VAR = bsStatic("mimetypes");

    settings = Parse_config_file(config_file);
    check(settings != NULL, "Error parsing config file: %s.", config_file);

    rc = Config_setup(db_file);
    check(rc == 0, "Failed to configure config db: %s", db_file);

    rc = AST_walk(settings, Server_load);
    check(rc == 0, "Failed to process the config file: %s", config_file);

    Value *set = AST_get(settings, settings, &SETTINGS_VAR, VAL_HASH);

    if(set) {
        rc = AST_walk_hash(settings, set, Settings_load);
        check(rc == 0, "Failed to load the settings. Aborting.");
    }

    rc = Mimetypes_import();
    check(rc == 0, "Failed to import default mimetypes.");

    Value *mime = AST_get(settings, settings, &MIMETYPES_VAR, VAL_HASH);
    if(mime) {
        AST_walk_hash(settings, mime, Mimetypes_load);
        check(rc == 0, "Failed to load the mimetypes. Aborting.");
    }

    rc = Config_commit();
    check(rc == 0, "Failed to commit config db: %s", db_file);

    AST_destroy(settings);
    DB_close();
    return 0;
error:
    AST_destroy(settings);
    DB_close();
    return -1;
}



