import simplejson as json
from mongrel2 import handler
import wsutil
import sys
import time

sender_id = "82209006-86FF-4982-B5EA-D1E29E55D480"

conn = handler.Connection(sender_id, "tcp://127.0.0.1:9990",
                          "tcp://127.0.0.1:9989")

CONNECTION_TIMEOUT=5

closingMessages={}

#logf=open('echo.log','wb')
#logf=open('/dev/null','wb')
logf=sys.stdout

def abortConnection(conn,req,reason='none'):
    conn.reply(req,'')
    print >>logf,'abort',reason

while True:
    now=time.time()
    logf.flush()
    for k,(t,uuid) in closingMessages.items():
        if now > t+CONNECTION_TIMEOUT:
            conn.send(uuid,k,'')
    try:
        req = conn.recv()
    except:
        print "FAILED RECV"
        continue


    #print "ID", req.conn_id
    if req.headers.get('METHOD') == 'GET':
        responseCode=wsutil.challenge(req.headers.get('sec-websocket-key'))
        response="HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: %s\r\n\r\n"%responseCode
        #print response
        conn.reply(req,response)
        continue

    if req.is_disconnect():
        #print "DISCONNECTED", req.conn_id

        continue

    if req.headers.get('METHOD') != 'WEBSOCKET':
        print 'METHOD is Not GET or WEBSOCKET',req.headers.get('METHOD')
        conn.reply(req,'')
        continue

    try:
        #print 'headers',req.headers
        flags = int(req.headers.get('FLAGS'),16)
        fin = flags&0x80==0x80
        rsvd=flags & 0x70
        opcode=flags & 0xf
        wsdata = req.body
        #print fin,rsvd,opcode,len(wsdata),wsdata
        #logf.write('\n')
    except:
        #print "Unable to decode FLAGS"
        abortConnection(conn,req,'WS decode failed')
        #continue

    if rsvd != 0:
        abortConnection(conn,req,'reserved non-zero')
        continue

    if opcode not in wsutil.opcodes:
        abortConnection(conn,req,'Unknown opcode')
        continue
        
    if (opcode & 0x8) != 0:
        if opcode ==wsutil.OP_PING:
            opcode = wsutil.OP_PONG
            conn.reply(req,wsutil.frame(wsdata,opcode))
        if opcode == wsutil.OP_CLOSE:
            if req.conn_id in closingMessages:
                del closingMessages[req.conn_id]
                conn.reply(req,'')
            else:
                conn.reply(req,wsutil.frame(wsdata,opcode))
                conn.reply(req,'')
        continue

    if opcode == wsutil.OP_PONG:
        continue # We don't send pings, so ignore pongs
    if(opcode == wsutil.OP_TEXT):
        try:
            wsdata.decode('utf-8')
        except:
            abortConnection(conn,req,'invalid UTF')
            continue
    conn.reply(req,wsutil.frame(wsdata,opcode))
