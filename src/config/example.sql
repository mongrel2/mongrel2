/** 
  Sample Mongrel2 configuration.  This is so you can see how a
  server could be configured "in the raw".  There would be a Python or Ruby
  (or other) library for doing most of this easier, and then a Web interface.
  For now, I'm looking for feedback on whether this makes sense.  Consider
  this a usability test, so if you can't figure this out then let me 
  know.
*/


/* This just creates the main database, you wouldn't have this always. */
.read src/config/config.sql

/* Creates a server that we'll be attaching configurations to.
   Servers always create a new process and always chroot to the root
   directory rather than trying to juggle file permissions.  They also
   drop priv to match the owner of the target directory.  This is much
   simpler and more secure.
 */
INSERT INTO server (id, name, uuid, chroot, access_log, error_log, default_type)  VALUES (
    0, 
    'mongrel2.org', 
    'A7BB3AD2-212B-4298-9BFE-D003C63AC02C',
    '/var/www/mongrel2',
    '/logs/access.log',
    '/logs/error.log',
    'text/plain');

/* Different listeners for getting requests from browsers. Notice some of these will be
   listeners for when our 0mq PUB/SUB server replies with chat messages.
 */
INSERT INTO listener (id, server_id, specification, socket_type) VALUES (0, 0, 'tcp://0.0.0.0:80', 'HTTP');
INSERT INTO listener (id, server_id, specification, socket_type) VALUES (1, 0, 'tcp://0.0.0.0:80', 'JS');
INSERT INTO listener (id, server_id, specification, socket_type) VALUES (2, 0, 'tcp://127.0.0.1:5556', 'SUB');

/* Three handlers for different types of requests. Notice that the JS socket
 is also a handler and a listener, Mongrel2 will know that you mean to send out
 on JS socket.
 */
INSERT INTO handler (id, specification, socket_type) VALUES (0, 'tcp://127.0.0.1:5555', 'PUB');
INSERT INTO handler (id, specification, socket_type) VALUES (1, 'tcp://0.0.0.0:80', 'JS');
INSERT INTO handler (id, specification, socket_type) VALUES (2, 'file://html', 'FILE');

/* These filters would be for keeping track of async pub/sub messages.
   They will tag requests from the JS socket after authenticating, and then
   any messages from the 0mq backend will be matched to registered users.
 */
INSERT INTO filter (id, name) VALUES (0, 'identify_request');
INSERT INTO filter (id, name) VALUES (1, 'match_response');

/* Sets up a jssocket to 0mq PUB/SUB async operation, tracking the identity and response.
   First one takes from the JS listener, sends it through the identify_request filter, then 
     sends it to the 0mq handler as a PUB message.
   Second one takes from the 0mq SUB backend, matches the message to who should get it,
     then sends it out on that JS socket.
*/
INSERT INTO route (path, listener_id, filter_id, handler_id) VALUES ('/chat', 1, 0, 0);
INSERT INTO route (path, listener_id, filter_id, handler_id) VALUES ('/chat', 2, 1, 1);

/* Sets up the usual HTTP file handler, doesn't have a filter. */
INSERT INTO route (path, listener_id, filter_id, handler_id) VALUES ('/', 0, NULL, 2);


