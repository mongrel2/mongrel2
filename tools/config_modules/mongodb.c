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

static mongo connexion;
static mongo *conn = NULL;
static bstring dbname;

const struct tagbstring server_token = bsStatic("srv");
const struct tagbstring shard_token = bsStatic("shard");

typedef enum {
    MONGO_CONNECTION_UNKNOWN,
    MONGO_CONNECTION_SERVER,
    MONGO_CONNECTION_SHARD
} mongo_connection_method_t;
/*
 *  Init the config system from a path string.
 *  Some example of mongodb description to server or shard cluster:
 *      server:localhost:mongrel2_collection
 *      server:localhost@27017:mongrel2
 *      shard:srv1;srv2:mongrel2
 *      shard:srv1@27017;srv2@27018:mongrel2
 *      shard:srv1;srv2;srv3;srv4:mongrel2
 */
int config_init(const char *path)
{
    int status;
    struct bstrList *tokens;
    struct bstrList *ips;
    mongo_connection_method_t conn_type = MONGO_CONNECTION_UNKNOWN;
    
    // Explode the path
    bstring dbspec = bfromcstr(path);
    tokens = bsplit(dbspec, ':');
    if (tokens->qty != 3) {
        log_err("Invalid database specification format.");
        bdestroy(dbspec);
        bstrListDestroy(tokens);
        return -1;
    }
    
    // Get connection type
    // use biseqcstr ??
    if (bstrcmp(tokens->entry[0], &server_token)) {
        conn_type = MONGO_CONNECTION_SERVER;
    } else if (bstrcmp(tokens->entry[0], &shard_token)) {
        conn_type = MONGO_CONNECTION_SHARD;
    }
    if (conn_type == MONGO_CONNECTION_UNKNOWN) {
        log_err("Unknown connection type.");
        bdestroy(dbspec);
        bstrListDestroy(tokens);
        return -1;
    }
    
    // Get database name
    dbname = tokens->entry[2];
    status = bconchar(dbname, '.');
    
    // Get host:port
    ips = bsplit(tokens->entry[1], ';');
    if ((tokens->qty != 1 && conn_type == MONGO_CONNECTION_SERVER) ||
        (tokens->qty < 2 && conn_type == MONGO_CONNECTION_SHARD))
    {
        log_err("Invalid number of mongoDB host definition.");
        bdestroy(dbspec);
        bstrListDestroy(tokens);
        bdestroy(dbname);
        bstrListDestroy(ips);
        return -1;
    }
    
    bdestroy(dbspec);
    bstrListDestroy(tokens);
         
    // Connect to mongo
    if (conn_type == MONGO_CONNECTION_SERVER) {
        struct bstrList *host_port = bsplit(ips->entry[0], ':');
        char *ip = bstr2cstr (host_port->entry[0], '\0');
        char *sPort = bstr2cstr (host_port->entry[0], '\0');
        int iPort = atoi(sPort);
    
        status = mongo_connect(&connexion, ip, iPort);
        bcstrfree(ip);
        bcstrfree(sPort);
        if (status != MONGO_OK) {
            log_err("Connection fail to mongoDB configuration server.");
            return -1;
        }
        conn = &connexion;
    } else if (conn_type == MONGO_CONNECTION_SHARD) {
    
    }
    
    
    return 0;
}

/*
 *  Close the connection with the configuration server
 */
void config_close()
{
    if (conn) {
        debug("Destroy mongoDB configuration server connection.");
        mongo_destroy(conn);
    }
}

tns_value_t *mongo_cursor_to_tns_value(mongo_cursor *cursor, bson *fields)
{
    tns_value_t *ret = NULL;

    ret = tns_new_list();
    
    // For each rows
    while (mongo_cursor_next(cursor) == MONGO_OK) {
        bson_iterator fields_iterator[1];
        
        //debug ("create row list");
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
            if (type == BSON_EOO) {
                break;
            }
            
            type = bson_find(cursor_iterator, mongo_cursor_bson(cursor), bson_iterator_key(fields_iterator));
            switch (type) {
                case BSON_STRING:
                    debug ("add string");
                    string_data = bson_iterator_string(cursor_iterator);
                    el = tns_parse_string(string_data, strlen(string_data));
                    break;
                    
                case BSON_BOOL:
                    debug ("add bool");
                    bool_data = bson_iterator_bool(cursor_iterator);
                    el = (bool_data) ? tns_get_true() : tns_get_false();
                    break;
                    
                case BSON_INT:
                    debug ("add int");
                    int_data = bson_iterator_int(cursor_iterator);
                    el = tns_new_integer(int_data);
                    break;
                   
                // convert double to int, to fix later 
                case BSON_DOUBLE:
                    debug ("add fake double");
                    int_data = (int)bson_iterator_double(cursor_iterator);
                    el = tns_new_integer(int_data);
                    break;

                case BSON_EOO:
                    debug ("End of object.");
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
        
        debug ("add row");
        tns_add_to_list(ret, row);
    }
    
    return ret;
}

tns_value_t *fetch_data(bstring collection_name, bson *fields, bson *query)
{
    int status;
    tns_value_t *ret = NULL;
    char *mongo_collection_name;
    mongo_cursor cursor[1];

    bstring collection = bstrcpy(dbname);
    status = bconcat(collection, collection_name);
    if (status != BSTR_OK) {
        log_err("bconcat failed");
        return NULL;
    }
    mongo_collection_name = bstr2cstr(collection, '\0');
    
    printf("%s\n", mongo_collection_name);
    
    mongo_cursor_init(cursor, conn, mongo_collection_name);
    mongo_cursor_set_query(cursor, query);
    mongo_cursor_set_fields(cursor, fields);
    bcstrfree (mongo_collection_name);

    ret = mongo_cursor_to_tns_value(cursor, fields);

    mongo_cursor_destroy(cursor);
  
    return ret;
}

tns_value_t *config_load_handler(int handler_id)
{
    tns_value_t *res = NULL;
    bson query[1], fields[1];

    debug("Loading handler");

    bstring collection = bfromcstr("handler");

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
    
    res = fetch_data(collection, fields, query);
    
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

    bstring collection = bfromcstr("proxy");

    bson_init(query);
    bson_append_int(query, "id", proxy_id);
    bson_finish(query);
    
    bson_init(fields);
    bson_append_int(fields, "id", 1);
    bson_append_int(fields, "addr", 1);
    bson_append_int(fields, "port", 1);
    bson_finish(fields);
    
    res = fetch_data(collection, fields, query);
    
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

    bstring collection = bfromcstr("directory");

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
    
    res = fetch_data(collection, fields, query);
    
    bdestroy(collection);
    bson_destroy(fields);
    bson_destroy(query);
    
    return res;
}

tns_value_t *config_load_routes(int host_id, int server_id)
{
    tns_value_t *res = NULL;
    bson query[1], fields[1];

    debug("Loading route");

    bstring collection = bfromcstr("route");

    bson_init(query);
    bson_append_int(query, "host_id", host_id);
    bson_finish(query);
    
    bson_init(fields);
    bson_append_int(fields, "id", 1);
    bson_append_int(fields, "path", 1);
    bson_append_int(fields, "target_id", 1);
    bson_append_int(fields, "target_type", 1);
    bson_finish(fields);
    
    res = fetch_data(collection, fields, query);
    
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

    bstring collection = bfromcstr("host");

    bson_init(query);
    bson_append_int(query, "server_id", server_id);
    bson_finish(query);
    
    bson_init(fields);
    bson_append_int(fields, "id", 1);
    bson_append_int(fields, "name", 1);
    bson_append_int(fields, "matching", 1);
    bson_append_int(fields, "server_id", 1);
    bson_finish(fields);
    
    res = fetch_data(collection, fields, query);
    
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

    bstring collection = bfromcstr("server");

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
    
    res = fetch_data(collection, fields, query);
    
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

    bstring collection = bfromcstr("mimetype");

    bson_init(query);
    bson_finish(query);
    
    bson_init(fields);
    bson_append_int(fields, "id", 1);
    bson_append_int(fields, "extension", 1);
    bson_append_int(fields, "mimetype", 1);
    bson_finish(fields);
    
    res = fetch_data(collection, fields, query);
    
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

    bstring collection = bfromcstr("setting");

    bson_init(query);
    bson_finish(query);
    
    bson_init(fields);
    bson_append_int(fields, "id", 1);
    bson_append_int(fields, "key", 1);
    bson_append_int(fields, "value", 1);
    bson_finish(fields);
    
    res = fetch_data(collection, fields, query);
    
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

    bstring collection = bfromcstr("filter");

    bson_init(query);
    bson_append_int(query, "server_id", server_id);
    bson_finish(query);
    
    bson_init(fields);
    bson_append_int(fields, "id", 1);
    bson_append_int(fields, "filter", 1);
    bson_append_int(fields, "settings", 1);
    bson_finish(fields);
    
    res = fetch_data(collection, fields, query);
    
    bdestroy(collection);
    bson_destroy(fields);
    bson_destroy(query);
    
    return res;
}

