import simplejson as json
from mongrel2 import handler
import wsutil
import sys
import time
import re

sender_id = "82209006-86FF-4982-B5EA-D1E29E55D480"

conn = handler.Connection(sender_id, "tcp://127.0.0.1:9990",
                          "tcp://127.0.0.1:9989")

CONNECTION_TIMEOUT=5

closingMessages={}

badUnicode=re.compile(u'[\ud800-\udfff]')

logf=open('echo.log','wb')
#logf=open('/dev/null','wb')
#logf=sys.stdout

def abortConnection(conn,req,reason='none',code=None):
    #print 'abort',conn,req,reason,code
    if code is not None:
        #print "Closing cleanly\n"
        conn.reply_websocket(req,code+reason,opcode=wsutil.OP_CLOSE)
        closingMessages[req.conn_id]=(time.time(),req.sender)
    else:
        conn.reply(req,'')
    print >>logf,'abort',code,reason

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


    if req.is_disconnect():
        #print "DISCONNECTED", req.conn_id
        continue

    if req.headers.get('METHOD') == 'WEBSOCKET_HANDSHAKE':
        #print "HANDSHAKE"
        conn.reply(req,
                '\r\n'.join([
                    "HTTP/1.1 101 Switching Protocols",
                    "Upgrade: websocket",
                    "Connection: Upgrade",
                    "Sec-WebSocket-Accept: %s\r\n\r\n"])%req.body)
        continue

    if req.headers.get('METHOD') != 'WEBSOCKET':
        print 'METHOD is Not WEBSOCKET:',req.headers#,req.body
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
        abortConnection(conn,req,'reserved non-zero',
                wsutil.CLOSE_PROTOCOL_ERROR)
        continue

    if opcode == wsutil.OP_CLOSE:
        if req.conn_id in closingMessages:
            del closingMessages[req.conn_id]
            conn.reply(req,'')
        else:
            conn.reply_websocket(req,wsdata,opcode)
            conn.reply(req,'')
        continue
    if req.conn_id in closingMessages:
        continue

    if opcode not in wsutil.opcodes:
        abortConnection(conn,req,'Unknown opcode',
                wsutil.CLOSE_PROTOCOL_ERROR)
        continue
        
    if (opcode & 0x8) != 0:
        if opcode ==wsutil.OP_PING:
            opcode = wsutil.OP_PONG
            conn.reply_websocket(req,wsdata,opcode)

        continue

    if opcode == wsutil.OP_PONG:
        continue # We don't send pings, so ignore pongs
    if(opcode == wsutil.OP_TEXT):
        try:
            x=wsdata.decode('utf-8')
            #Thank you for not fixing python issue8271 in 2.x :(
            if badUnicode.search(x):
                raise UnicodeError('Surrogates not allowed')
            #for c in x:
                #if (0xd800 <= ord(c) <= 0xdfff):
                    #raise UnicodeError('Surrogates not allowed')
        except:
            abortConnection(conn,req,'invalid UTF', wsutil.CLOSE_BAD_DATA)
            continue
    conn.reply_websocket(req,wsdata,opcode)
