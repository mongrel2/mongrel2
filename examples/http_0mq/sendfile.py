from mongrel2 import handler
from mongrel2 import tnetstrings
import json
from uuid import uuid4

# ZMQ 2.1.x broke how PUSH/PULL round-robin works so each process
# needs it's own id for it to work
sender_id = uuid4().hex

conn = handler.Connection(sender_id, "tcp://127.0.0.1:9999",
                          "tcp://127.0.0.1:9998")
while True:
    print "WAITING FOR REQUEST"

    req = conn.recv()

    print "REQ received"

    if req.is_disconnect():
        print "DISCONNECT"
        continue

    if req.headers.get("killme", None):
        print "They want to be killed."
        response = ""
    else:
        response = "<pre>\nSENDER: %r\nIDENT:%r\nPATH: %r\nHEADERS:%r\nBODY:%r</pre>" % (
            req.sender, req.conn_id, req.path, 
            json.dumps(req.headers), req.body)

        print response

    f=open('hello.txt','r')
    f.seek(0,2)
    length=f.tell()
    payload = {'code': 200, 'status': 'OK', 'body': ''}
    headers={}
    headers['Content-Length'] =  length
    payload['headers'] = "\r\n".join('%s: %s' % (k,v) for k,v in
                                     headers.items())
    conn.reply(req, handler.HTTP_FORMAT % payload)
    conn.deliver(req.sender, ['X',req.conn_id ],
            tnetstrings.dump(['sendfile','examples/http_0mq/hello.txt']))

    if req.should_close():
        conn.reply(req,'')

