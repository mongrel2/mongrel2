INSERT INTO server (uuid, access_log, error_log, chroot, default_host, port) 
    VALUES (
        'AC1F8236-5919-4696-9D40-0F38DE9E5861',
        '/mongrel2/logs/access.log',
        '/mongrel2/logs/error.log',
        '/var/www',
        'mongrel2.org',
        6767);

INSERT INTO host (server_id, name, matching)
    VALUES (
        last_insert_rowid(),
        'mongrel2.org',
        '(.*).mongrel2.org'
    );

INSERT INTO handler (send_spec, send_ident, recv_spec, recv_ident)
    VALUES (
        'tcp://127.0.0.1:9999',
        '54c6755b-9628-40a4-9a2d-cc82a816345e',
        'tcp://127.0.0.1:9998',
        ''
    );

INSERT INTO route (path, host_id, target_id, target_type)
    VALUES (
        "@chat", 
        (select id from host where name="mongrel2.org"),
        last_insert_rowid(),
        "handler"
    );

INSERT INTO proxy (addr, port)
    VALUES 
    (
        '127.0.0.1',
        8080
    );


INSERT INTO route (path, host_id, target_id, target_type)
    VALUES (
        "/chat/",
        (select id from host where name="mongrel2.org"),
        last_insert_rowid(),
        "proxy"
    );



INSERT INTO proxy (addr, port)
    VALUES 
    (
        '127.0.0.1',
        8080
    );


INSERT INTO route (path, host_id, target_id, target_type)
    VALUES (
        "/",
        (select id from host where name="mongrel2.org"),
        last_insert_rowid(),
        "proxy"
    );

INSERT INTO directory (base) VALUES ("/tests/.+.json");

INSERT INTO route (path, host_id, target_id, target_type)
    VALUES (
        "/tests/",
        (select id from host where name="mongrel2.org"),
        last_insert_rowid(),
        "dir"
    );
