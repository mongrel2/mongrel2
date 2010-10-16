#!/usr/bin/env python

import sys
import socket
from base64 import b64decode

try:
    import json
except:
    import simplejson as json

import getpass

host = sys.argv[1]
port = int(sys.argv[2])

def read_msg():
    reply = ""

    ch = CONN.recv(1)
    while ch != '\0':
        reply += ch
        ch = CONN.recv(1)

    return json.loads(b64decode(reply))

def post_msg(data):
    msg = '@chat {"type": "msg", "msg": "%s"}\x00' % (data)
    CONN.send(msg)

print "Connecting to %s:%d" % (host, port)
CONN = socket.socket()
CONN.connect((host, port))
USER = getpass.getuser()

post_msg("connect")

while True:
    try:
        reply = read_msg()

        if 'msg' in reply and reply['msg']:
            print reply['msg']

        if reply['type'] == "prompt":
            msg = raw_input(reply['pchar'])
            post_msg(msg)

        if reply['type'] == 'exit':
            sys.exit(0)

    except EOFError:
        print "\nBye."
        break

