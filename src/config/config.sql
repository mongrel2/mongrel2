DROP TABLE server;
DROP TABLE listener;
DROP TABLE handler;
DROP TABLE filter;
DROP TABLE route;

DROP TABLE socket_type;
DROP TABLE statistic;
DROP TABLE mimetype;
DROP TABLE setting;

CREATE TABLE socket_type (name TEXT);
INSERT INTO socket_type VALUES ('HTTP');
INSERT INTO socket_type VALUES ('PUB');
INSERT INTO socket_type VALUES ('SUB');
INSERT INTO socket_type VALUES ('REQ');
INSERT INTO socket_type VALUES ('REP');
INSERT INTO socket_type VALUES ('DOWNSTREAM');
INSERT INTO socket_type VALUES ('UPSTREAM');
INSERT INTO socket_type VALUES ('RAW');
INSERT INTO socket_type VALUES ('JS');
INSERT INTO socket_type VALUES ('FILE');

CREATE TABLE server (id INTEGER PRIMARY KEY, name TEXT, uuid TEXT,
    access_log TEXT, error_log TEXT, default_type TEXT DEFAULT 'text/plain',
    maintenance BOOLEAN DEFAULT false, chroot TEXT DEFAULT '/var/www/mongrel2'
);

CREATE TABLE listener (id INTEGER PRIMARY KEY, server_id INTEGER, specification TEXT, socket_type TEXT);

CREATE TABLE handler (id INTEGER PRIMARY KEY, specification TEXT, socket_type TEXT);

CREATE TABLE filter (id INTEGER PRIMARY KEY, name TEXT);


CREATE TABLE route (id INTEGER PRIMARY KEY, path TEXT, listener_id INTEGER, filter_id INTEGER, handler_id INTEGER);


CREATE TABLE setting (id SERIAL, 
    other_id INTEGER, other_type TEXT, key TEXT, value TEXT,
    primary key (id, other_type, other_id));

CREATE TABLE statistic (id SERIAL, 
    other_type TEXT, other_id INTEGER, name text,
    sum REAL, sumsq REAL, n INTEGER, min REAL, max REAL, mean REAL, sd REAL,
    primary key (other_type, other_id, name));


CREATE TABLE mimetype (id INTEGER PRIMARY KEY, mimetype TEXT, extension TEXT);

.read src/config/mimetypes.sql

