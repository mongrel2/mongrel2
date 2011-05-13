CONN = socket.socket()
CONN.connect((host, port))

def read_msg():
    reply = ""

    ch = CONN.recv(1)
    while ch != '\0':
        reply += ch
        ch = CONN.recv(1)

    return json.loads(b64decode(reply))

def post_msg(data):
    msg = '@bbs %s\x00' % (
        json.dumps({'type': 'msg', 'msg': data}))
    CONN.send(msg)
