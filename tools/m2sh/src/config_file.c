/**
 *
 * Copyright (c) 2010, Zed A. Shaw and Mongrel2 Project Contributors.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 * 
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 * 
 *     * Neither the name of the Mongrel2 Project, Zed A. Shaw, nor the names
 *       of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written
 *       permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

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

struct tagbstring CACHE_TTL = bsStatic("cache_ttl");

int Dir_load(tst_t *settings, tst_t *params)
{
    const char *base = AST_str(settings, params, "base", VAL_QSTRING);
    tns_value_t *res = NULL;

    check(base[0] != '/', "Don't start the base with / in %s; it will fail when not in chroot.", base);
    check(base[strlen(base) - 1] == '/', "End directory base with / in %s or it won't work right.'", base);

    res = DB_exec(bdata(&DIR_SQL), base,
            AST_str(settings, params, "index_file", VAL_QSTRING),
            AST_str(settings, params, "default_ctype", VAL_QSTRING));
    check(res != NULL, "Invalid database, couldn't query for directory: %s", base);

    if(tst_search(params, bdata(&CACHE_TTL), blength(&CACHE_TTL))) {
        const char *cache_ttl = AST_str(settings, params, "cache_ttl", VAL_NUMBER);

        if(cache_ttl && cache_ttl[0] != '0') {
            tns_value_destroy(res);
            res = DB_exec(bdata(&DIR_CACHE_TTL_SQL), cache_ttl);
            check(res != NULL, "Invalid database, couldn't set cache_ttl in directory: %s", base);
        }
    }

    tns_value_destroy(res);
    return DB_lastid();

error:
    if(res) tns_value_destroy(res);
    return -1;
}

struct tagbstring RAW_PAYLOAD = bsStatic("raw_payload");
struct tagbstring PROTOCOL = bsStatic("protocol");

int Handler_load(tst_t *settings, tst_t *params)
{
    const char *send_spec = AST_str(settings, params, "send_spec", VAL_QSTRING);
    tns_value_t *res = NULL;

    res = DB_exec(bdata(&HANDLER_SQL),
            send_spec,
            AST_str(settings, params, "send_ident", VAL_QSTRING),
            AST_str(settings, params, "recv_spec", VAL_QSTRING),
            AST_str(settings, params, "recv_ident", VAL_QSTRING));
    check(res != NULL, "Failed to load Handler: %s", send_spec);

    if(tst_search(params, bdata(&RAW_PAYLOAD), blength(&RAW_PAYLOAD))) {
        const char *raw_payload = AST_str(settings, params, "raw_payload", VAL_NUMBER);

        if(raw_payload && raw_payload[0] == '1') {
            DB_exec(bdata(&HANDLER_RAW_SQL));
            check(res != NULL, "Invalid database, can't set raw payload.");
            tns_value_destroy(res);
        }
    }

    if(tst_search(params, bdata(&PROTOCOL), blength(&PROTOCOL))) {
        const char *protocol = AST_str(settings, params, "protocol", VAL_QSTRING);
        protocol = protocol != NULL ? protocol : "json";

        tns_value_destroy(res);
        res = DB_exec(bdata(&HANDLER_PROTOCOL_SQL), protocol);
        check(res != NULL, "Invalid SQL with your protocol setting: '%s'", protocol);
    }
    
    tns_value_destroy(res);
    return DB_lastid();

error:

    if(res) tns_value_destroy(res);
    return -1;
}


int Proxy_load(tst_t *settings, tst_t *params)
{
    const char *addr = AST_str(settings, params, "addr", VAL_QSTRING);
    const char *port = AST_str(settings, params, "port", VAL_NUMBER);

    tns_value_t *res = DB_exec(bdata(&PROXY_SQL), addr, port);
    check(res != NULL, "Failed to load Proxy: %s:%s", addr, port);

    tns_value_destroy(res);
    return DB_lastid();

error:
    if(res) tns_value_destroy(res);
    return -1;
}

int Mimetypes_import()
{
    char *zErrMsg = NULL;
    int rc = sqlite3_exec(CONFIG_DB, bdata(&MIMETYPES_DEFAULT_SQL), NULL, NULL, &zErrMsg);
    check(rc == SQLITE_OK, "Failed to load initial schema: %s", zErrMsg);

    return 0;
error:
    if(zErrMsg) sqlite3_free(zErrMsg);
    return -1;
}

int Mimetypes_load(tst_t *settings, Pair *pair)
{
    const char *ext = bdata(Pair_key(pair));
    tns_value_t *res = NULL;
    Value *val = Pair_value(pair);
    check(val, "Error loading Mimetype %s", bdata(Pair_key(pair)));

    res = DB_exec(bdata(&MIMETYPE_SQL),
            ext, ext, bdata(val->as.string->data));

    check(res != NULL, "Failed to add mimetype: %s=%s",
            ext, bdata(val->as.string->data));

    tns_value_destroy(res);
    return 0;

error:
    if(res) tns_value_destroy(res);
    return -1;
}

int Settings_load(tst_t *settings, Pair *pair)
{
    const char *name = bdata(Pair_key(pair));
    tns_value_t *res = NULL;
    Value *val = Pair_value(pair);
    check(val, "Error loading Setting %s", bdata(Pair_key(pair)));

    res = DB_exec(bdata(&SETTING_SQL), name, bdata(val->as.string->data));

    check(res != NULL, "Failed to add setting: %s=%s",
            name, bdata(val->as.string->data));

    tns_value_destroy(res);
    return 0;

error:
    if(res) tns_value_destroy(res);
    return -1;
}


int Route_load(tst_t *settings, Pair *pair)
{
    const char *name = bdata(Pair_key(pair));
    tns_value_t *res = NULL;
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

    res = DB_exec(bdata(&ROUTE_SQL), name, HOST_ID, cls->id, bdata(type));
    check(res != NULL, "Failed to intialize route.");

    tns_value_destroy(res);
    bdestroy(type);
    return 0;

error:
    if(res) tns_value_destroy(res);
    if(type) bdestroy(type);
    return -1;
}

struct tagbstring MATCHING_PARAM = bsStatic("matching");

int Plugin_load(tst_t *settings, Value *val, bstring sql, const char *type)
{
    Class *cls = val->as.cls;
    tns_value_t *res = NULL;
    struct tagbstring SETTINGS_VAR = bsStatic("settings");
    char *converted_settings = NULL;
    tns_value_t *filter_tns = NULL;

    CONFIRM_TYPE(type);

    const char *name = AST_str(settings, cls->params, "name", VAL_QSTRING);
    check(name != NULL, "You must set a name for the filter.");

    Value *filter_settings = AST_get(settings, cls->params, &SETTINGS_VAR, VAL_HASH);
    check(filter_settings != NULL, "Invalid or missing settings for Filter '%s'", name);

    filter_tns = AST_tns_convert_value(settings, filter_settings);
    check(filter_tns, "Failed to convert settings for filter '%s'", name);
    check(tns_get_type(filter_tns) == tns_tag_dict,
            "Settings for Filter '%s' must be a dict type.", name);

    size_t len = 0;
    converted_settings = tns_render(filter_tns, &len);
    tns_value_destroy(filter_tns);

    check(converted_settings != NULL && len > 0,
            "Failed to convert final Filter settings to tnetstring for Filter '%s'",
            name);

    res = DB_exec(bdata(sql), SERVER_ID, name, converted_settings);
    check(res != NULL, "Failed to store Filter: '%s'", name);

    tns_value_destroy(res);
    free(converted_settings);

    return 0;
error:
    if(converted_settings) free(converted_settings);
    if(filter_tns) tns_value_destroy(filter_tns);
    return -1;
}
int Filter_load(tst_t *settings, Value *val)
{
    return Plugin_load(settings,val,&FILTER_SQL,"Filter");
}
int Xrequest_load(tst_t *settings, Value *val)
{
    return Plugin_load(settings,val,&XREQUEST_SQL,"Xrequest");
}


int Host_load(tst_t *settings, Value *val)
{
    tns_value_t *res = NULL;

    CONFIRM_TYPE("Host");

    Class *cls = val->as.cls;
    struct tagbstring ROUTES_VAR = bsStatic("routes");

    const char *name = AST_str(settings, cls->params, "name", VAL_QSTRING);
    const char *matching = name; // default to this then change it
    check(name, "No name set for Host.");

    if(tst_search(cls->params, bdata(&MATCHING_PARAM), blength(&MATCHING_PARAM))) {
        // specified matching so use that
        matching = AST_str(settings, cls->params, bdata(&MATCHING_PARAM), VAL_QSTRING);
    }

    res = DB_exec(bdata(&HOST_SQL), SERVER_ID, name, matching);
    check(res != NULL, "Failed to store Host: %s", name);

    cls->id = HOST_ID = DB_lastid();

    Value *routes = AST_get(settings, cls->params, &ROUTES_VAR, VAL_HASH);
    check(routes, "Didn't find any routes for %s", name);

    AST_walk_hash(settings, routes, Route_load);

    tns_value_destroy(res);
    return 0;

error:
    if(res) tns_value_destroy(res);
    return -1;
}

struct tagbstring BIND_ADDR = bsStatic("bind_addr");
struct tagbstring USE_SSL = bsStatic("use_ssl");

int Server_load(tst_t *settings, Value *val)
{
    tns_value_t *res = NULL;

    CONFIRM_TYPE("Server");
    Class *cls = val->as.cls;
    struct tagbstring HOSTS_VAR = bsStatic("hosts");
    struct tagbstring FILTERS_VAR = bsStatic("filters");
    struct tagbstring XREQUESTS_VAR = bsStatic("xrequests");
    const char *bind_addr = NULL;
    const char *use_ssl = NULL;

    if(tst_search(cls->params, bdata(&BIND_ADDR), blength(&BIND_ADDR))) {
        bind_addr = AST_str(settings, cls->params, bdata(&BIND_ADDR), VAL_QSTRING);
    } else {
        bind_addr = "0.0.0.0";
    }

    if(tst_search(cls->params, bdata(&USE_SSL), blength(&USE_SSL))) {
        use_ssl = AST_str(settings, cls->params, bdata(&USE_SSL), VAL_NUMBER);
    } else {
        use_ssl = "0";
    }

    res = DB_exec(bdata(&SERVER_SQL),
            AST_str(settings, cls->params, "uuid", VAL_QSTRING),
            AST_str(settings, cls->params, "access_log", VAL_QSTRING),
            AST_str(settings, cls->params, "error_log", VAL_QSTRING),
            AST_str(settings, cls->params, "pid_file", VAL_QSTRING),
            AST_str(settings, cls->params, "chroot", VAL_QSTRING),
            AST_str(settings, cls->params, "default_host", VAL_QSTRING),
            AST_str(settings, cls->params, "name", VAL_QSTRING),
            bind_addr,
            AST_str(settings, cls->params, "port", VAL_NUMBER),
            use_ssl
            );
    check(res != NULL, "Failed to exec SQL: %s", bdata(&SERVER_SQL));

    cls->id = SERVER_ID = DB_lastid();

    // setup the hosts
    Value *hosts = AST_get(settings, cls->params, &HOSTS_VAR, VAL_LIST);
    check(hosts != NULL, "Could not find Server.hosts setting in host %s:%s",
            AST_str(settings, cls->params, "uuid", VAL_QSTRING),
            AST_str(settings, cls->params, "name", VAL_QSTRING));

    AST_walk_list(settings, hosts->as.list, Host_load);

    // setup the filters
    Value *filters = AST_get(settings, cls->params, &FILTERS_VAR, VAL_LIST);

    if(filters != NULL) {
        AST_walk_list(settings, filters->as.list, Filter_load);
    }

    Value *xrequests = AST_get(settings, cls->params, &XREQUESTS_VAR, VAL_LIST);

    if(xrequests != NULL) {
        AST_walk_list(settings, xrequests->as.list, Xrequest_load);
    }

    tns_value_destroy(res);
    return 0;

error:
    if(res) tns_value_destroy(res);
    return -1;
}

static inline int Config_setup(const char *db_file)
{
    char *zErrMsg = NULL;
    DB_init(db_file);
    tns_value_t *res = DB_exec("begin");
    check(res != NULL, "Couldn't start transaction.");

    int rc = sqlite3_exec(CONFIG_DB, bdata(&CONFIG_SCHEMA), NULL, NULL, &zErrMsg);
    check(rc == SQLITE_OK, "Failed to load initial schema: %s", zErrMsg);

    tns_value_destroy(res);
    return 0;

error:
    if(res) tns_value_destroy(res);
    if(zErrMsg) sqlite3_free(zErrMsg);
    return -1;
}


static inline int Config_commit()
{
    return DB_exec("commit") == NULL ? -1 : 0;
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



