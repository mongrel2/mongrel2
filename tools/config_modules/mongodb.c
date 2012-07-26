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

#include <dbg.h>
#include <config/module.h>
#include "tnetstrings_impl.h"

#include "mongo.h"

static mongo connexion[1];
static bstring dbname;

#define MONGODB_HOST_PORT_SEPARATOR '@'
#define MONGODB_IP_SEPARATOR        ';'
#define MONGODB_TOKEN_SEPARATOR     ':'
#define MONGODB_TOKEN_SHARD         "shard"
#define MONGODB_TOKEN_SERVER        "srv"

static int config_init_index(void)
{
    bson key[1];
    log_info ("Ensure that the database have index");

    bson_init(key);
    bson_append_int(key, "id", 1);
    bson_finish(key);
    mongo_create_index(connexion, "mongrel2.handler", key, MONGO_INDEX_BACKGROUND, NULL);
    bson_destroy(key);

    bson_init(key);
    bson_append_int(key, "id", 1);
    bson_finish(key);
    mongo_create_index(connexion, "mongrel2.proxy", key, MONGO_INDEX_BACKGROUND, NULL);
    bson_destroy(key);

    bson_init(key);
    bson_append_int(key, "id", 1);
    bson_finish(key);
    mongo_create_index(connexion, "mongrel2.directory", key, MONGO_INDEX_BACKGROUND, NULL);
    bson_destroy(key);

    bson_init(key);
    bson_append_int(key, "host_id", 1);
    bson_finish(key);
    mongo_create_index(connexion, "mongrel2.route", key, MONGO_INDEX_BACKGROUND, NULL);
    bson_destroy(key);

    bson_init(key);
    bson_append_int(key, "server_id", 1);
    bson_finish(key);
    mongo_create_index(connexion, "mongrel2.host", key, MONGO_INDEX_BACKGROUND, NULL);
    bson_destroy(key);

    bson_init(key);
    bson_append_int(key, "uuid", 1);
    bson_finish(key);
    mongo_create_index(connexion, "mongrel2.server", key, MONGO_INDEX_BACKGROUND, NULL);
    bson_destroy(key);

    bson_init(key);
    bson_append_int(key, "server_id", 1);
    bson_finish(key);
    mongo_create_index(connexion, "mongrel2.filter", key, MONGO_INDEX_BACKGROUND, NULL);
    bson_destroy(key);

    return 0;
}

static int config_init_server(struct bstrList *tokens)
{
    int ret = -1;
    int status, port = MONGO_DEFAULT_PORT;
    char *ip = NULL, *sport = NULL;
    struct bstrList *host = NULL;
    struct bstrList *ips = NULL;

    dbname = bstrcpy(tokens->entry[2]);
    check (dbname->mlen > 0, "Database name can't be empty");

    ips = bsplit(tokens->entry[1], MONGODB_IP_SEPARATOR);
    check (ips->qty == 1, "In server connection mode, you must provide only one host."); 

    host = bsplit(ips->entry[0], MONGODB_HOST_PORT_SEPARATOR);
    check (host != NULL, "Error on bsplit"); 

    switch (host->qty) {
        case 2:
            sport = bstr2cstr (host->entry[1], '\0');
            check (sport != NULL, "Error on bstr2cstr");
            port = atoi(sport);

        case 1:
            ip = bstr2cstr (host->entry[0], '\0');
            check (ip != NULL, "Error on bstr2cstr");
            break;           

        default:
            sentinel("Something is cooking.");
    }

    log_info ("Connecting to server %s:%d", ip, port);
    status = mongo_connect(connexion, ip, port);
    check(status == MONGO_OK, "Connection fail to mongoDB configuration server.");

    ret = 0;

error:
    bstrListDestroy(host);
    bstrListDestroy(ips);
    bcstrfree(ip);
    bcstrfree(sport);
    return ret;
}

static int config_init_shard(struct bstrList *tokens)
{
    int ret = -1;
    int i, status, port;
    bstring shardname;
    char *shard = NULL;
    char *ip = NULL, *sport = NULL;
    struct bstrList *host = NULL;
    struct bstrList *ips = NULL;

    shardname = tokens->entry[1];
    check(shardname->slen > 0, "Shard name can't be empty");

    dbname = bstrcpy(tokens->entry[3]);
    check (dbname->mlen > 0, "Database name can't be empty");

    ips = bsplit(tokens->entry[2], MONGODB_IP_SEPARATOR);
    check (ips->qty > 1, "In server connection mode, you must provide only one host."); 

    host = bsplit(ips->entry[0], ':');
    check (host != NULL, "Error on bsplit"); 

    shard = bstr2cstr (shardname, '\0');
    check (shard != NULL, "Error on bstr2cstr");
    mongo_replset_init(connexion, shard);
    log_info("Connecting to shard \"%s\"", shard);
    
    i = ips->qty;
    while(--i >= 0) {
        port = MONGO_DEFAULT_PORT;
        host = bsplit(ips->entry[i], MONGODB_HOST_PORT_SEPARATOR);
        check (host != NULL, "Error on bsplit"); 

        switch (host->qty) {
            case 2:
                sport = bstr2cstr (host->entry[1], '\0');
                check (sport != NULL, "Error on bstr2cstr");
                port = atoi(sport);

            case 1:
                ip = bstr2cstr (host->entry[0], '\0');
                check (ip != NULL, "Error on bstr2cstr");
                break;           

            default:
                sentinel("Something is cooking.");
        }
        mongo_replset_add_seed(connexion, ip, port);
        log_info("Add seed %s:%d", ip, port);
        bcstrfree(ip), ip = NULL;
        bcstrfree(sport), sport = NULL;
    }

    status = mongo_replset_connect(connexion);
    check(status == MONGO_OK, "Connection fail to mongoDB configuration shard.");

    ret = 0;

error:
    bstrListDestroy(host);
    bstrListDestroy(ips);
    bcstrfree(shard);
    bcstrfree(ip);
    bcstrfree(sport);
    return ret;
}

/*
 *  Init the config system from a path string.
 *  Some example of mongodb description to server or shard cluster:
 *      server:localhost:mongrel2_collection
 *      server:localhost@27017:mongrel2
 *      shard:shardName:srv1;srv2:mongrel2
 *      shard:shardName:srv1@27017;srv2@27018:mongrel2
 *      shard:shardName:srv1;srv2;srv3;srv4:m2
 */
int config_init(const char *path)
{
    int status, ret = -1;
    struct bstrList *tokens = NULL;

    log_info("Init mongoDB configuration module");

    bstring dbspec = bfromcstr(path);
    check(dbspec != NULL, "Can't read path.");

    tokens = bsplit(dbspec, MONGODB_TOKEN_SEPARATOR);
    check(tokens != NULL, "Can't split the path.");

    if (biseqcstr(tokens->entry[0], MONGODB_TOKEN_SERVER) == 1) {
        check(tokens->qty == 3, "Invalid database specification format.");
        ret = config_init_server(tokens);
    } else if (biseqcstr(tokens->entry[0], MONGODB_TOKEN_SHARD) == 1) {
        check(tokens->qty == 4, "Invalid database specification format.");
        ret = config_init_shard(tokens);
    } else {
        sentinel("Unknown connection type.");
    }
    check(ret == 0, "Error during connection.");

    status = bconchar(dbname, '.');
    check (status != BSTR_ERR, "Error on bconchar");

    ret = config_init_index();    
    check(ret == 0, "Error during setup index.");

error:
    bdestroy(dbspec);
    bstrListDestroy(tokens);
    return ret;
}

/*
 *  Close the connection with the configuration server
 */
void config_close()
{
    log_info("Close mongoDB configuration module");
    mongo_destroy(connexion);
    bdestroy(dbname);
}

tns_value_t *mongo_cursor_to_tns_value(mongo_cursor *cursor, bson *fields)
{
    tns_value_t *ret = NULL;

    ret = tns_new_list();
    
    while (mongo_cursor_next(cursor) == MONGO_OK) {
        bson_iterator fields_iterator[1];
        
        tns_value_t *row = tns_new_list();
        bson_iterator_init(fields_iterator, fields);
        
        // For each fields in the query
        do {
            bson_iterator cursor_iterator[1];
            bson_type type;
            const char *string_data;
            int int_data;
            int bool_data;
            tns_value_t *el = NULL;
            
            type = bson_iterator_next(fields_iterator);
            if (type == BSON_EOO) { // EOO: End of object
                break;
            }
            
            type = bson_find(cursor_iterator, mongo_cursor_bson(cursor), bson_iterator_key(fields_iterator));
            switch (type) {
                case BSON_STRING:
                    string_data = bson_iterator_string(cursor_iterator);
                    el = tns_parse_string(string_data, strlen(string_data));
                    break;
                    
                case BSON_BOOL:
                    bool_data = bson_iterator_bool(cursor_iterator);
                    el = (bool_data) ? tns_get_true() : tns_get_false();
                    break;
                    
                case BSON_INT:
                    int_data = bson_iterator_int(cursor_iterator);
                    el = tns_new_integer(int_data);
                    break;

                default:
                    log_err("Not supported BSON type (%d)", type);
            }
            
            if (el) {
                tns_add_to_list(row, el);
                el = NULL;
            } else {
                // Go to next row
                break;
            }
        } while (1);
        
        tns_add_to_list(ret, row);
    }
    
    return ret;
}

tns_value_t *fetch_data(bstring collection_name, bson *fields, bson *query)
{
    int status;
    tns_value_t *ret = NULL;
    char *mongo_collection_name = NULL;
    mongo_cursor cursor[1];

    errno = 0;

    bstring collection = bstrcpy(dbname);
    check_mem(collection);

    status = bconcat(collection, collection_name);
    check(status == BSTR_OK, "Error on bconcat");

    mongo_collection_name = bstr2cstr(collection, '\0');
    check_mem(mongo_collection_name);
    
    mongo_cursor_init(cursor, connexion, mongo_collection_name);
    mongo_cursor_set_query(cursor, query);
    mongo_cursor_set_fields(cursor, fields);

    ret = mongo_cursor_to_tns_value(cursor, fields);

error:
    bdestroy(collection);
    bcstrfree(mongo_collection_name);
    mongo_cursor_destroy(cursor);
    return ret;
}
tns_value_t *config_load_handler(int handler_id)
{
    tns_value_t *res = NULL;
    bson query[1], fields[1];

    debug("Loading handler");

    bson_init(query);
    bson_append_int(query, "id", handler_id);
    bson_finish(query);
    
    bson_init(fields);
    bson_append_int(fields, "id", 1);
    bson_append_int(fields, "send_spec", 1);
    bson_append_int(fields, "send_ident", 1);
    bson_append_int(fields, "recv_spec", 1);
    bson_append_int(fields, "recv_ident", 1);
    bson_append_int(fields, "raw_payload", 1);
    bson_append_int(fields, "protocol", 1);
    bson_finish(fields);

    bstring collection = bfromcstr("handler");
    check_mem(collection);

    res = fetch_data(collection, fields, query);

error: 
    bdestroy(collection);
    bson_destroy(fields);
    bson_destroy(query);
    
    return res;
}

tns_value_t *config_load_proxy(int proxy_id)
{
    tns_value_t *res = NULL;
    bson query[1], fields[1];

    debug("Loading proxy");

    bson_init(query);
    bson_append_int(query, "id", proxy_id);
    bson_finish(query);
    
    bson_init(fields);
    bson_append_int(fields, "id", 1);
    bson_append_int(fields, "addr", 1);
    bson_append_int(fields, "port", 1);
    bson_finish(fields);

    bstring collection = bfromcstr("proxy");
    check_mem(collection);
    
    res = fetch_data(collection, fields, query);

error:   
    bdestroy(collection);
    bson_destroy(fields);
    bson_destroy(query);
    
    return res;
}

tns_value_t *config_load_dir(int dir_id)
{
    tns_value_t *res = NULL;
    bson query[1], fields[1];

    debug("Loading directory");

    bson_init(query);
    bson_append_int(query, "id", dir_id);
    bson_finish(query);
    
    bson_init(fields);
    bson_append_int(fields, "id", 1);
    bson_append_int(fields, "base", 1);
    bson_append_int(fields, "index_file", 1);
    bson_append_int(fields, "default_ctype", 1);
    bson_append_int(fields, "cache_ttl", 1);
    bson_finish(fields);

    bstring collection = bfromcstr("directory");
    check_mem(collection);
   
    res = fetch_data(collection, fields, query);

error:    
    bdestroy(collection);
    bson_destroy(fields);
    bson_destroy(query);
    
    return res;
}

tns_value_t *config_load_routes(int host_id, int server_id)
{
    tns_value_t *res = NULL;
    bson query[1], fields[1];

    // server_id is useless
    (void)server_id;

    debug("Loading route");

    bson_init(query);
    bson_append_int(query, "host_id", host_id);
    bson_finish(query);
    
    bson_init(fields);
    bson_append_int(fields, "id", 1);
    bson_append_int(fields, "path", 1);
    bson_append_int(fields, "target_id", 1);
    bson_append_int(fields, "target_type", 1);
    bson_finish(fields);

    bstring collection = bfromcstr("route");
    check_mem(collection);
    
    res = fetch_data(collection, fields, query);

error:    
    bdestroy(collection);
    bson_destroy(fields);
    bson_destroy(query);
    
    return res;
}

tns_value_t *config_load_hosts(int server_id)
{
    tns_value_t *res = NULL;
    bson query[1], fields[1];

    debug("Loading host");

    bson_init(query);
    bson_append_int(query, "server_id", server_id);
    bson_finish(query);
    
    bson_init(fields);
    bson_append_int(fields, "id", 1);
    bson_append_int(fields, "name", 1);
    bson_append_int(fields, "matching", 1);
    bson_append_int(fields, "server_id", 1);
    bson_finish(fields);

    bstring collection = bfromcstr("host");
    check_mem(collection);
    
    res = fetch_data(collection, fields, query);

error:    
    bdestroy(collection);
    bson_destroy(fields);
    bson_destroy(query);
    
    return res;
}

tns_value_t *config_load_server(const char *uuid)
{
    tns_value_t *res = NULL;
    bson query[1], fields[1];

    debug("Loading server");

    bson_init(query);
    bson_append_string(query, "uuid", uuid);
    bson_finish(query);
    
    bson_init(fields);
    bson_append_int(fields, "id", 1);
    bson_append_int(fields, "uuid", 1);
    bson_append_int(fields, "default_host", 1);
    bson_append_int(fields, "bind_addr", 1);
    bson_append_int(fields, "port", 1);
    bson_append_int(fields, "chroot", 1);
    bson_append_int(fields, "access_log", 1);
    bson_append_int(fields, "error_log", 1);
    bson_append_int(fields, "pid_file", 1);
    bson_append_int(fields, "use_ssl", 1);
    bson_finish(fields);

    bstring collection = bfromcstr("server");
    check_mem(collection);

    res = fetch_data(collection, fields, query);

error:    
    bdestroy(collection);
    bson_destroy(fields);
    bson_destroy(query);
    
    return res;
}

tns_value_t *config_load_mimetypes()
{
    tns_value_t *res = NULL;
    bson query[1], fields[1];

    debug("Loading mimetypes");

    bson_init(query);
    bson_finish(query);
    
    bson_init(fields);
    bson_append_int(fields, "id", 1);
    bson_append_int(fields, "extension", 1);
    bson_append_int(fields, "mimetype", 1);
    bson_finish(fields);

    bstring collection = bfromcstr("mimetype");
    check_mem(collection);

    res = fetch_data(collection, fields, query);

error:
    bdestroy(collection);
    bson_destroy(fields);
    bson_destroy(query);
    
    return res;
}

tns_value_t *config_load_settings()
{
    tns_value_t *res = NULL;
    bson query[1], fields[1];

    debug("Loading setting");

    bson_init(query);
    bson_finish(query);
    
    bson_init(fields);
    bson_append_int(fields, "id", 1);
    bson_append_int(fields, "key", 1);
    bson_append_int(fields, "value", 1);
    bson_finish(fields);

    bstring collection = bfromcstr("setting");
    check_mem(collection);

    res = fetch_data(collection, fields, query);

error:
    bdestroy(collection);
    bson_destroy(fields);
    bson_destroy(query);
    
    return res;
}

tns_value_t *config_load_filters(int server_id)
{
    tns_value_t *res = NULL;
    bson query[1], fields[1];

    debug("Loading filter");

    bson_init(query);
    bson_append_int(query, "server_id", server_id);
    bson_finish(query);
    
    bson_init(fields);
    bson_append_int(fields, "id", 1);
    bson_append_int(fields, "filter", 1);
    bson_append_int(fields, "settings", 1);
    bson_finish(fields);

    bstring collection = bfromcstr("filter");
    check_mem(collection);

    res = fetch_data(collection, fields, query);

error:
    bdestroy(collection);
    bson_destroy(fields);
    bson_destroy(query);
    
    return res;
}

