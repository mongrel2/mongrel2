.read src/config/config.sql

INSERT INTO server (id, name, uuid, chroot, access_log, error_log, default_type)  VALUES (
    0, 'mongrel2.org', 'A7BB3AD2-212B-4298-9BFE-D003C63AC02C',
    '/var/www/mongrel2', '/logs/access.log', '/logs/error.log', 'text/plain');

INSERT INTO listener (id, server_id, specification, socket_type) VALUES (0, 0, 'tcp://0.0.0.0:80', 'HTTP');
INSERT INTO listener (id, server_id, specification, socket_type) VALUES (1, 0, 'tcp://0.0.0.0:80', 'JS');
INSERT INTO listener (id, server_id, specification, socket_type) VALUES (2, 0, 'tcp://127.0.0.1:5556', 'SUB');

INSERT INTO handler (id, specification, socket_type) VALUES (0, 'tcp://127.0.0.1:5555', 'PUB');
INSERT INTO handler (id, specification, socket_type) VALUES (1, 'tcp://0.0.0.0:80', 'JS');


INSERT INTO filter (id, name) VALUES (0, 'identify_request');
INSERT INTO filter (id, name) VALUES (1, 'match_response');

INSERT INTO route (path, listener_id, filter_id, handler_id) VALUES ('/', 1, 0, 0);
INSERT INTO route (path, listener_id, filter_id, handler_id) VALUES ('/', 2, 1, 1);


