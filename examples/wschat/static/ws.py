from mongrel2 import handler
import base64
import hashlib
import json

def wsChallenge(v):
    try:
        x=hashlib.sha1(v+"258EAFA5-E914-47DA-95CA-C5AB0DC85B11")
        return base64.b64encode(x.digest())
    except:
        return ""

sender_id = "59619F68-8E14-4743-81B2-038E7294541D"

conn = handler.Connection(sender_id, "tcp://127.0.0.1:9997",
                          "tcp://127.0.0.1:9996")

activeConnections={}


while True:
    print "WAITING FOR REQUEST"

    req = conn.recv()
    response=""

    if req.is_disconnect():
        print "DISCONNECT "+req.conn_id
        if req.conn_id in activeConnections:
            del activeConnections[req.conn_id]
        continue

    if req.headers.get('METHOD') == 'WEBSOCKET':
        print "WS Message received "+req.conn_id
        for v in activeConnections.values():
            #Just broadcast frame to everyone
            #We could use send to be faster, but this is just a demo
            conn.reply(v,req.body)
        continue

    if req.headers.get('sec-websocket-key'):
        responseCode=wsChallenge(req.headers.get('sec-websocket-key'))
        response="HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: %s\r\n\r\n"%responseCode
        print response
        activeConnections[req.conn_id] = req

    conn.reply(req, response)

