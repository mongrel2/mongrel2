import zmq
import time

sender_id = "82209006-86FF-4982-B5EA-D1E29E55D481"


ctx = zmq.Context()
reqs = ctx.socket(zmq.SUB)
reqs.setsockopt(zmq.SUBSCRIBE, "")
reqs.connect("tcp://127.0.0.1:9997")

resp = ctx.socket(zmq.PUB)
resp.connect("tcp://127.0.0.1:9996")
resp.setsockopt(zmq.IDENTITY, sender_id)

class Request(object):

    def __init__(self, ident, headers, body):
        self.ident = ident
        self.headers = headers
        self.body = body


def parse_request(msg):
    ident, rest = msg.split(' ', 1)
    length, rest = rest.split(':', 1)
    length = int(length)
    headers = rest[0:length]
    headers = headers.split('\01')[:-1]
    headers = dict(zip(headers[::2], headers[1::2]))
    body = rest[length+1:]

    return Request(ident, headers, body)


while True:
    req = parse_request(reqs.recv())

    response = "<pre>\nIDENT:%r\nHEADERS:%r\nBODY:%r</pre>" % (req.ident, req.headers,
                                                  req.body)

    resp.send(req.ident + " HTTP/1.1 200 OK\r\nContent-Length: %d\r\n\r\n%s" % (
        len(response), response))

