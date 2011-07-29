from mongrel2 import handler
import json
from uuid import uuid4

# ZMQ 2.1.x broke how PUSH/PULL round-robin works so each process
# needs it's own id for it to work
sender_id = uuid4().hex

conn = handler.Connection(sender_id, "tcp://127.0.0.1:9997",
                          "tcp://127.0.0.1:9996")
while True:
    print "WAITING FOR REQUEST"

    req = conn.recv()

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

    conn.reply_http(req, response)

