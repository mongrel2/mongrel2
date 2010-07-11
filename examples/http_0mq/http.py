import zmq
import time
import sys
import simplejson as json

sender_id = "82209006-86FF-4982-B5EA-D1E29E55D481"


ctx = zmq.Context()
reqs = ctx.socket(zmq.SUB)
reqs.setsockopt(zmq.SUBSCRIBE, "")
reqs.connect("tcp://127.0.0.1:9997")

resp = ctx.socket(zmq.PUB)
resp.connect("tcp://127.0.0.1:9996")
resp.setsockopt(zmq.IDENTITY, sender_id)

def parse_netstring(ns):
    len, rest = ns.split(':', 1)
    len = int(len)
    return rest[:len], rest[len+1:]


class Request(object):

    def __init__(self, sender, conn_id, path, headers, body):
        self.sender = sender
        self.path = path
        self.conn_id = conn_id
        self.headers = headers
        self.body = body


    @staticmethod
    def parse(msg):
        sender, conn_id, path, rest = msg.split(' ', 3)
        headers, rest = parse_netstring(rest)
        body, _ = parse_netstring(rest)

        headers = json.loads(headers)

        print "SENDER:", sender, "ID:", conn_id, "PATH:", path, "HEADERS:", headers, "BODY:", body

        return Request(sender, conn_id, path, headers, body)



while True:
    req = Request.parse(reqs.recv())

    response = "<pre>\nSENDER: %r\nIDENT:%r\nPATH: %r\nHEADERS:%r\nBODY:%r</pre>" % (
        req.sender, req.conn_id, req.path, 
        json.dumps(req.headers), req.body)

    print response, "\n"

    resp.send(req.conn_id + " HTTP/1.1 200 OK\r\nContent-Length: %d\r\n\r\n%s" % (
        len(response), response))

