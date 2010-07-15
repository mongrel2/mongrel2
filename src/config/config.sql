begin transaction;

DROP TABLE IF EXISTS server;
DROP TABLE IF EXISTS host;
DROP TABLE IF EXISTS handler;
DROP TABLE IF EXISTS proxy;
DROP TABLE IF EXISTS route;
DROP TABLE IF EXISTS statistic;
DROP TABLE IF EXISTS mimetype;
DROP TABLE IF EXISTS setting;
DROP TABLE IF EXISTS directory;

CREATE TABLE server (id INTEGER PRIMARY KEY,
    uuid TEXT,
    access_log TEXT,
    error_log TEXT,
    chroot TEXT DEFAULT '/var/www',
    default_host INTEGER,
    port INTEGER);

CREATE TABLE host (id INTEGER PRIMARY KEY, 
    server_id INTEGER,
    default_type TEXT DEFAULT 'text/plain',
    maintenance BOOLEAN DEFAULT 0,
    name TEXT,
    matching TEXT);

CREATE TABLE handler (id INTEGER PRIMARY KEY,
    send_spec TEXT, 
    send_ident TEXT,
    recv_spec TEXT,
    recv_ident TEXT);

CREATE TABLE proxy (id INTEGER PRIMARY KEY,
    addr TEXT,
    port INTEGER);

CREATE TABLE directory (id INTEGER PRIMARY KEY,
    base TEXT, prefix TEXT, index_file TEXT, default_ctype TEXT);

CREATE TABLE route (id INTEGER PRIMARY KEY,
    path TEXT,
    reversed BOOLEAN DEFAULT 0,
    host_id INTEGER,
    target_id INTEGER,
    target_type TEXT);


CREATE TABLE setting (id SERIAL, 
    other_id INTEGER,
    other_type TEXT,
    key TEXT,
    value TEXT,
    primary key (id, other_type, other_id));


CREATE TABLE statistic (id SERIAL, 
    other_type TEXT,
    other_id INTEGER,
    name text,
    sum REAL,
    sumsq REAL,
    n INTEGER,
    min REAL,
    max REAL,
    mean REAL,
    sd REAL,
    primary key (other_type, other_id, name));


CREATE TABLE mimetype (id INTEGER PRIMARY KEY, mimetype TEXT, extension TEXT);

commit;
