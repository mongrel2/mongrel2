import simplejson as json
from mongrel2 import handler
import wsutil
import sys
import time

sender_id = "82209006-86FF-4982-B5EA-D1E29E55D480"

conn = handler.Connection(sender_id, "tcp://127.0.0.1:9990",
                          "tcp://127.0.0.1:9989")

CONNECTION_TIMEOUT=5

incomingMessages = {}
closingMessages={}

#logf=open('echo.log','wb')
logf=open('/dev/null','wb')

def abortConnection(conn,req,reason='none'):
    if req.conn_id in incomingMessages:
        del incomingMessages[req.conn_id]
    conn.reply(req,'')
    print >>logf,'abort',reason

def hexdump(s,f=sys.stdout):
    count=0
    for i in s:
        f.write(hex(ord(i)))
        f.write(' ')
        count+=1
        if(count==20):
            count=0
            f.write('\n')

while True:
    now=time.time()
    for k,(t,uuid) in closingMessages.items():
        if now > t+CONNECTION_TIMEOUT:
            conn.send(uuid,k,'')
    try:
        req = conn.recv()
    except:
        print "FAILED RECV"
        continue


    print "ID", req.conn_id
    print incomingMessages.keys()
    if req.headers.get('METHOD') == 'GET':
        responseCode=wsutil.challenge(req.headers.get('sec-websocket-key'))
        response="HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: %s\r\n\r\n"%responseCode
        #print response
        conn.reply(req,response)
        continue

    if req.is_disconnect():
        #print "DISCONNECTED", req.conn_id

        if req.conn_id in incomingMessages:
            del incomingMessages[req.conn_id]
        continue

    if req.headers.get('METHOD') != 'WEBSOCKET':
        print 'METHOD is Not GET or WEBSOCKET',req.headers.get('METHOD')
        conn.reply(req,'')
        continue

    try:
        fin,rsvd,opcode,wsdata = wsutil.deframe(req.body)
        #logf.write('packet %s %s %s:\n'%(fin,rsvd,opcode))
        #hexdump(req.body,logf)
        #logf.write('\n')
    except:
        #hexdump(req.body[:14])
        #print "WS Frame decode failed"
        abortConnection(conn,req,'WS decode failed')
        continue

    if rsvd != 0:
        abortConnection(conn,req,'reserved non-zero')
        continue
    if opcode == 0 and req.conn_id not in incomingMessages:
        abortConnection(conn,req,'continuation of nonexistant message')
        #print req.conn_id
        #print incomingMessages
        continue

    if opcode not in wsutil.opcodes:
        abortConnection(conn,req,'Unknown opcode')
        continue
        
    if (opcode & 0x8) != 0:
#Control frames
        if not fin:
            abortConnection(conn,req,'fragmented control packet')
            continue
        if len(wsdata) > 125:
            abortConnection(conn,req,'too long control packet')
            continue
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

    #print "FRAME:"
    #hexdump(req.body[:14])
    #print "DATA:",wsdata
    wsdata=[wsdata]
    start=[]
    if req.conn_id in incomingMessages:
        if opcode != 0:
            abortConnection(conn,req,"Non-control frame is not continuation")
            continue
        start,opcode=incomingMessages[req.conn_id]
        del incomingMessages[req.conn_id]
    wsdata = start+wsdata
    if fin:
        if opcode == wsutil.OP_PONG:
            continue # We don't send pings, so ignore pongs
        if(opcode == wsutil.OP_TEXT):
            try:
                ''.join(wsdata).decode('utf-8')
            except:
                abortConnection(conn,req,'invalid UTF')
                continue
        #print 'Message complete: '
        #print ''.join(wsdata)
        #print
        #hexdump(wsutil.frame(''.join(wsdata))[:14])
        conn.reply(req,wsutil.frame(''.join(wsdata),opcode))
    else:
        #print 'got chunk for',req.conn_id
        incomingMessages[req.conn_id]=wsdata,opcode
