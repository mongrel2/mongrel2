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

#include "constants.h"

struct tagbstring CONFIG_SCHEMA = bsStatic(
"DROP TABLE IF EXISTS server;\n"
"DROP TABLE IF EXISTS host;\n"
"DROP TABLE IF EXISTS handler;\n"
"DROP TABLE IF EXISTS proxy;\n"
"DROP TABLE IF EXISTS route;\n"
"DROP TABLE IF EXISTS statistic;\n"
"DROP TABLE IF EXISTS mimetype;\n"
"DROP TABLE IF EXISTS setting;\n"
"DROP TABLE IF EXISTS directory;\n"
"DROP TABLE IF EXISTS filter;\n"
"DROP TABLE IF EXISTS xrequest;\n"
"\n"
"CREATE TABLE server (id INTEGER PRIMARY KEY,\n"
"    uuid TEXT,\n"
"    access_log TEXT,\n"
"    error_log TEXT,\n"
"    chroot TEXT DEFAULT '/var/www',\n"
"    pid_file TEXT,\n"
"    control_port TEXT DEFAULT '',\n"
"    default_host TEXT,\n"
"    name TEXT DEFAULT '',\n"
"    bind_addr TEXT DEFAULT \"0.0.0.0\",\n"
"    port INTEGER,\n"
"    use_ssl INTEGER default 0);\n"
"\n"
"CREATE TABLE host (id INTEGER PRIMARY KEY, \n"
"    server_id INTEGER,\n"
"    maintenance BOOLEAN DEFAULT 0,\n"
"    name TEXT,\n"
"    matching TEXT);\n"
"\n"
"CREATE TABLE handler (id INTEGER PRIMARY KEY,\n"
"    send_spec TEXT, \n"
"    send_ident TEXT,\n"
"    recv_spec TEXT,\n"
"    recv_ident TEXT,\n"
"   raw_payload INTEGER DEFAULT 0,\n"
"   protocol TEXT DEFAULT 'json');\n"
"\n"
"CREATE TABLE proxy (id INTEGER PRIMARY KEY,\n"
"    addr TEXT,\n"
"    port INTEGER);\n"
"\n"
"CREATE TABLE directory (id INTEGER PRIMARY KEY,"
"   base TEXT,"
"   index_file TEXT,"
"   default_ctype TEXT,"
"   cache_ttl INTEGER DEFAULT 0);"
"\n"
"CREATE TABLE route (id INTEGER PRIMARY KEY,\n"
"    path TEXT,\n"
"    reversed BOOLEAN DEFAULT 0,\n"
"    host_id INTEGER,\n"
"    target_id INTEGER,\n"
"    target_type TEXT);\n"
"\n"
"CREATE TABLE setting (id INTEGER PRIMARY KEY, key TEXT, value TEXT);\n"
"\n"
"CREATE TABLE statistic (id SERIAL, \n"
"    other_type TEXT,\n"
"    other_id INTEGER,\n"
"    name TEXT,\n"
"    sum REAL,\n"
"    sumsq REAL,\n"
"    n INTEGER,\n"
"    min REAL,\n"
"    max REAL,\n"
"    mean REAL,\n"
"    sd REAL,\n"
"    primary key (other_type, other_id, name));\n"
"\n"
"CREATE TABLE mimetype (id INTEGER PRIMARY KEY, mimetype TEXT, extension TEXT);\n"
"\n"
"CREATE TABLE filter (id INTEGER PRIMARY KEY, \n"
"    server_id INTEGER, \n"
"    name TEXT, \n"
"    settings TEXT);\n"
"\n"
"CREATE TABLE xrequest (id INTEGER PRIMARY KEY, \n"
"    server_id INTEGER, \n"
"    name TEXT, \n"
"    settings TEXT);\n"
"\n"
"CREATE TABLE IF NOT EXISTS log(id INTEGER PRIMARY KEY,\n"
"    who TEXT,\n"
"    what TEXT,\n"
"    location TEXT,\n"
"    happened_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,\n"
"    how TEXT,\n"
"    why TEXT);");



struct tagbstring SERVER_SQL = bsStatic("INSERT INTO server (uuid, access_log, error_log, pid_file, control_port, chroot, default_host, name, bind_addr, port, use_ssl) VALUES (%Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %s, %s);");

struct tagbstring HOST_SQL = bsStatic("INSERT INTO host (server_id, name, matching) VALUES (%d, %Q, %Q);");

struct tagbstring FILTER_SQL = bsStatic("INSERT INTO filter (server_id, name, settings) VALUES (%d, %Q, %Q);");

struct tagbstring XREQUEST_SQL = bsStatic("INSERT INTO xrequest (server_id, name, settings) VALUES (%d, %Q, %Q);");

struct tagbstring SETTING_SQL = bsStatic("INSERT INTO setting (key, value) VALUES (%Q, %Q);");
struct tagbstring MIMETYPE_SQL = bsStatic("DELETE from mimetype where extension=%Q; INSERT INTO mimetype (extension, mimetype) VALUES (%Q, %Q);");

struct tagbstring DIR_SQL = bsStatic("INSERT INTO directory (base, index_file, default_ctype) VALUES (%Q, %Q, %Q);");
struct tagbstring DIR_CACHE_TTL_SQL = bsStatic("UPDATE directory SET cache_ttl=%Q WHERE id=last_insert_rowid();");

struct tagbstring PROXY_SQL = bsStatic("INSERT INTO proxy (addr, port) VALUES (%Q, %Q);");

struct tagbstring HANDLER_SQL = bsStatic("INSERT INTO handler (send_spec, send_ident, recv_spec, recv_ident) VALUES (%Q, %Q, %Q, %Q);");

struct tagbstring ROUTE_SQL = bsStatic("INSERT INTO route (path, host_id, target_id, target_type) VALUES (%Q, %d, %d, %Q);");

struct tagbstring HANDLER_RAW_SQL = bsStatic("UPDATE handler SET raw_payload=1 WHERE id=last_insert_rowid();");
struct tagbstring HANDLER_PROTOCOL_SQL = bsStatic("UPDATE handler SET protocol=%Q WHERE id=last_insert_rowid();");

// gotta love some evil :-)
#include "mimetypes.csql"
