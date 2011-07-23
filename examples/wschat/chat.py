import simplejson as json
from mongrel2 import handler
import hashlib
import base64

sender_id = "82209006-86FF-4982-B5EA-D1E29E55D481"

conn = handler.Connection(sender_id, "tcp://127.0.0.1:9999",
                          "tcp://127.0.0.1:9998")
users = {}
user_list = []


def wsChallenge(v):
    try:
        x=hashlib.sha1(v+"258EAFA5-E914-47DA-95CA-C5AB0DC85B11")
        return base64.b64encode(x.digest())
    except:
        return ""

def wsdeframe(data):
    #We rely on mongrel2 to have done the length calculation and read
    #Just the header and payload
    fin=(ord(data[0])&0x80)!=0
    masked=(ord(data[1])&0x80)!=0
    leng=ord(data[1])&0x7f
    if not masked:
        raise Exception("Packet not masked!")
    if not fin:
        raise Exception("Fragmentation not supported")
    payloadStart=6
    if leng == 126:
        payloadStart +=2
    elif leng == 127:
        payloadStart += 8
    maskKey=map(ord,data[payloadStart-4:payloadStart])
    dataOut=[]
    index=0
    for byte in data[payloadStart:]:
        dataOut.append(chr(ord(byte)^maskKey[index%4]))
        index+=1
    return "".join(dataOut)

def wsframe(data,opcode=1):
    header=''
    header+=chr(0x80|opcode)
    realLength=len(data)
    if realLength < 126:
        dummyLength=realLength
    elif realLength < 2**32:
        dummyLength = 126
    else:
        dummyLength=127
    header+=chr(dummyLength)
    if dummyLength == 127:
        header += chr(realLength >> 56 &0xff)
        header += chr(realLength >> 48 &0xff)
        header += chr(realLength >> 40 &0xff)
        header += chr(realLength >> 32 &0xff)
    if dummyLength == 126 or dummyLength == 127:
        header += chr(realLength >> 24 & 0xff)
        header += chr(realLength >> 16 & 0xff)
        header += chr(realLength >> 8 & 0xff)
        header += chr(realLength & 0xff)
    return header+data

while True:
    try:
        req = conn.recv()
    except:
        print "FAILED RECV"
        continue


    print "ID", req.conn_id
    if req.headers.get('METHOD') == 'GET':
        responseCode=wsChallenge(req.headers.get('sec-websocket-key'))
        response="HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: %s\r\n\r\n"%responseCode
        print response
        conn.reply(req,response)
        continue

    if req.is_disconnect():
        print "DISCONNECTED", req.conn_id

        if req.conn_id in users:
            data['user'] = users[req.conn_id]
            del users[req.conn_id]

        if len(users.keys()) > 0:
            conn.deliver(req.sender, users.keys(), wsframe(json.dumps(data)))
            user_list = [u[1] for u in users.items()]

    if req.headers.get('METHOD') != 'WEBSOCKET':
        print 'METHOD is Not GET or WEBSOCKET',req.headers.get('METHOD')
        continue

    try:
        wsdata = wsdeframe(req.body)
    except:
        print "WS Frame decode failed"
        req.reply('')
        continue
    print "DATA:",wsdata
    try:
        data = json.loads(wsdata)
    except:
        print "JSON decode failed"
        #conn.reply(req,'')
        continue

    if data["type"] == "join":
        conn.deliver_json(req.sender, users.keys(), data)
        users[req.conn_id] = data['user']
        user_list = [u[1] for u in users.items()]
        conn.reply(req, wsframe(json.dumps({'type': 'userList', 'users': user_list})))

    elif data["type"] == "disconnect":
        print "DISCONNECTED", req.conn_id

        if req.conn_id in users:
            data['user'] = users[req.conn_id]
            del users[req.conn_id]

        if len(users.keys()) > 0:
            conn.deliver(req.sender, users.keys(), wsframe(json.dumps(data)))
            user_list = [u[1] for u in users.items()]

    elif req.conn_id not in users:
        users[req.conn_id] = data['user']

    elif data['type'] == "msg":
        conn.deliver(req.sender, users.keys(), wsframe(json.dumps(data)))

    print "REGISTERED USERS:", len(users)

