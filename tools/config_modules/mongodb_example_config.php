<?php


$cfg = array(
    'server' => 'mongodb://localhost:27017',
    'options' => array('connect' => true),
    'database' => 'mongrel2',
);
$con = new \Mongo($cfg['server'], $cfg['options']);
$db = $con->selectDB($cfg['database']);

// Setting
$setting = array (
  'id' => new MongoInt32(1),
  'key' => 'upload:temp_store',
  'value' => '/mongrel2/tmp/mongrel2.upload.XXXXXX',
);
$db->setting->insert($setting, array('safe' => true));
$setting = array (
  'id' => new MongoInt32(2),
  'key' => 'limits.content_length',
  'value' => '2097152000',
);
$db->setting->insert($setting, array('safe' => true));

// Host
$host = array (
  'id' => new MongoInt32(1),
  'server_id' => new MongoInt32(1),
  'maintenance' => false,
  'name' => 'localhost',
  'matching' => 'localhost',
);
$db->host->insert($host, array('safe' => true));

// Mimetype
$mimetype = array (
  'id' => 2,
  'mimetype' => 'application/x-msbinderssss',
  'extension' => '.odbs',
);
$db->mimetype->insert($mimetype, array('safe' => true));
$mimetype = array (
  'id' => 1,
  'mimetype' => 'application/x-msbinder',
  'extension' => '.odb',
);
$db->mimetype->insert($mimetype, array('safe' => true));

// Route
$route = array (
  'id' => new MongoInt32(1),
  'path' => '/',
  'reversed' => false,
  'host_id' => new MongoInt32(1),
  'target_id' => new MongoInt32(1),
  'target_type' => 'handler',
);
$db->route->insert($route, array('safe' => true));

// Server
$server = array (
  'access_log' => '/mongrel2/logs/access.log',
  'bind_addr' => '0.0.0.0',
  'chroot' => './',
  'default_host' => 'localhost',
  'error_log' => '/mongrel2/logs/error.log',
  'id' => new MongoInt32(1),
  'name' => 'main',
  'pid_file' => '/mongrel2/run/mongrel2.pid',
  'port' => 6767,
  'use_ssl' => new MongoInt32(0),
  'uuid' => '971e0536-a92d-41e2-bea3-9af54bfd6fd9',
);
$db->server->insert($server, array('safe' => true));

// Handler
$handler = array (
  'id' => new MongoInt32(1),
  'send_spec' => 'tcp://127.0.0.1:9001',
  'send_ident' => 'aaaaaaaaaaaaaaaaaaaaaaaaaaaaa',
  'recv_spec' => 'tcp://127.0.0.1:9000',
  'recv_ident' => '',
  'raw_payload' => new MongoInt32(0),
  'protocol' => 'json',
);
$db->handler->insert($handler, array('safe' => true));

echo 'Done.';

