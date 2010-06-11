DROP TABLE route;
DROP TABLE handler;


CREATE TABLE route (id INTEGER PRIMARY KEY, path TEXT, target TEXT);

CREATE TABLE handler (id INTEGER PRIMARY KEY, name TEXT, style TEXT, addr TEXT);


INSERT INTO handler (name, style, addr) VALUES ("app", "REQ", "tcp://127.0.0.1:5000");
INSERT INTO handler (name, style, addr) VALUES ("web", "TCP", "tcp://127.0.0.1:5001");


INSERT INTO route (path, target) VALUES ("/", "app");
INSERT INTO route (path, target) VALUES ("/names", "app");
INSERT INTO route (path, target) VALUES ("/things", "web");
INSERT INTO route (path, target) VALUES ("/things", "app");
INSERT INTO route (path, target) VALUES ("/apples", "web");
INSERT INTO route (path, target) VALUES ("/oranges", "web");


