#include "constants.h"

struct tagbstring CONFIG_SCHEMA = bsStatic(
"DROP TABLE IF EXISTS server;"
"DROP TABLE IF EXISTS host;"
"DROP TABLE IF EXISTS handler;"
"DROP TABLE IF EXISTS proxy;"
"DROP TABLE IF EXISTS route;"
"DROP TABLE IF EXISTS statistic;"
"DROP TABLE IF EXISTS mimetype;"
"DROP TABLE IF EXISTS setting;"
"DROP TABLE IF EXISTS directory;"
""
"CREATE TABLE server (id INTEGER PRIMARY KEY,"
"    uuid TEXT,"
"    access_log TEXT,"
"    error_log TEXT,"
"    chroot TEXT DEFAULT '/var/www',"
"    pid_File TEXT,"
"    default_host INTEGER,"
"    name TEXT DEFAULT '',"
"    port INTEGER);"
""
"CREATE TABLE host (id INTEGER PRIMARY KEY, "
"    server_id INTEGER,"
"    maintenance BOOLEAN DEFAULT 0,"
"    name TEXT,"
"    matching TEXT);"
""
"CREATE TABLE handler (id INTEGER PRIMARY KEY,"
"    send_spec TEXT, "
"    send_ident TEXT,"
"    recv_spec TEXT,"
"    recv_ident TEXT);"
""
"CREATE TABLE proxy (id INTEGER PRIMARY KEY,"
"    addr TEXT,"
"    port INTEGER);"
""
"CREATE TABLE directory (id INTEGER PRIMARY KEY,"
"    base TEXT, index_file TEXT, default_ctype TEXT);"
""
"CREATE TABLE route (id INTEGER PRIMARY KEY,"
"    path TEXT,"
"    reversed BOOLEAN DEFAULT 0,"
"    host_id INTEGER,"
"    target_id INTEGER,"
"    target_type TEXT);"
""
"CREATE TABLE setting (id INTEGER PRIMARY KEY, key TEXT, value TEXT);"
""
"CREATE TABLE statistic (id SERIAL, "
"    other_type TEXT,"
"    other_id INTEGER,"
"    name text,"
"    sum REAL,"
"    sumsq REAL,"
"    n INTEGER,"
"    min REAL,"
"    max REAL,"
"    mean REAL,"
"    sd REAL,"
"    primary key (other_type, other_id, name));"
""
""
"CREATE TABLE mimetype (id INTEGER PRIMARY KEY, mimetype TEXT, extension TEXT);"
""
"CREATE TABLE IF NOT EXISTS log(id INTEGER PRIMARY KEY,"
"    who TEXT,"
"    what TEXT,"
"    location TEXT,"
"    happened_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
"    how TEXT,"
"    why TEXT);");



struct tagbstring SERVER_SQL = bsStatic("INSERT INTO server (uuid, access_log, error_log, pid_file, chroot, default_host, name, port) VALUES (%Q, %Q, %Q, %Q, %Q, %Q, %Q, %s);");

struct tagbstring HOST_SQL = bsStatic("INSERT INTO host (server_id, name, matching) VALUES (%d, %Q, %Q);");

struct tagbstring SETTING_SQL = bsStatic("INSERT INTO setting (key, value) VALUES (%Q, %Q);");

struct tagbstring DIR_SQL = bsStatic("INSERT INTO directory (base, index_file, default_ctype) VALUES (%Q, %Q, %Q);");

struct tagbstring PROXY_SQL = bsStatic("INSERT INTO proxy (addr, port) VALUES (%Q, %Q);");

struct tagbstring HANDLER_SQL = bsStatic("INSERT INTO handler (send_spec, send_ident, recv_spec, recv_ident) VALUES (%Q, %Q, %Q, %Q);");

struct tagbstring ROUTE_SQL = bsStatic("INSERT INTO route (path, host_id, target_id, target_type) VALUES (%Q, %d, %d, %Q);");

