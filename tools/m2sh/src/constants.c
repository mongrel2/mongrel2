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
"\n"
"CREATE TABLE server (id INTEGER PRIMARY KEY,\n"
"    uuid TEXT,\n"
"    access_log TEXT,\n"
"    error_log TEXT,\n"
"    chroot TEXT DEFAULT '/var/www',\n"
"    pid_File TEXT,\n"
"    default_host INTEGER,\n"
"    name TEXT DEFAULT '',\n"
"    port INTEGER);\n"
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
"    recv_ident TEXT);\n"
"\n"
"CREATE TABLE proxy (id INTEGER PRIMARY KEY,\n"
"    addr TEXT,\n"
"    port INTEGER);\n"
"\n"
"CREATE TABLE directory (id INTEGER PRIMARY KEY,\n"
"    base TEXT, index_file TEXT, default_ctype TEXT);\n"
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
"    name text,\n"
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
"CREATE TABLE IF NOT EXISTS log(id INTEGER PRIMARY KEY,\n"
"    who TEXT,\n"
"    what TEXT,\n"
"    location TEXT,\n"
"    happened_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,\n"
"    how TEXT,\n"
"    why TEXT);");



struct tagbstring SERVER_SQL = bsStatic("INSERT INTO server (uuid, access_log, error_log, pid_file, chroot, default_host, name, port) VALUES (%Q, %Q, %Q, %Q, %Q, %Q, %Q, %s);");

struct tagbstring HOST_SQL = bsStatic("INSERT INTO host (server_id, name, matching) VALUES (%d, %Q, %Q);");

struct tagbstring SETTING_SQL = bsStatic("INSERT INTO setting (key, value) VALUES (%Q, %Q);");
struct tagbstring MIMETYPE_SQL = bsStatic("DELETE from mimetype where extension=%Q; INSERT INTO mimetype (extension, mimetype) VALUES (%Q, %Q);");

struct tagbstring DIR_SQL = bsStatic("INSERT INTO directory (base, index_file, default_ctype) VALUES (%Q, %Q, %Q);");

struct tagbstring PROXY_SQL = bsStatic("INSERT INTO proxy (addr, port) VALUES (%Q, %Q);");

struct tagbstring HANDLER_SQL = bsStatic("INSERT INTO handler (send_spec, send_ident, recv_spec, recv_ident) VALUES (%Q, %Q, %Q, %Q);");

struct tagbstring ROUTE_SQL = bsStatic("INSERT INTO route (path, host_id, target_id, target_type) VALUES (%Q, %d, %d, %Q);");

// gotta love some evil :-)
#include "mimetypes.csql"
