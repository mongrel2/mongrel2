import zmq
import time
from mongrel2.request import Request
try:
    import json
except:
    import simplejson as json

CTX = zmq.Context()

HTTP_FORMAT = "HTTP/1.1 %(code)s %(status)s\r\n%(headers)s\r\n\r\n%(body)s"
MAX_IDENTS = 100

def http_response(body, code, status, headers):
    payload = {'code': code, 'status': status, 'body': body}
    headers['Content-Length'] = len(body)
    payload['headers'] = "\r\n".join('%s: %s' % (k,v) for k,v in
                                     headers.items())

    return HTTP_FORMAT % payload


class Connection(object):
    """
    A Connection object manages the connection between your handler
    and a Mongrel2 server (or servers).  It can receive raw requests
    or JSON encoded requests whether from HTTP or MSG request types,
    and it can send individual responses or batch responses either
    raw or as JSON.  It also has a way to encode HTTP responses
    for simplicity since that'll be fairly common.
    """

    def __init__(self, sender_id, sub_addr, pub_addr):
        """
        Your addresses should be the same as what you configured
        in the config.sqlite for Mongrel2 and are usually like 
        tcp://127.0.0.1:9998
        """
        self.sender_id = sender_id

        reqs = CTX.socket(zmq.UPSTREAM)
        reqs.connect(sub_addr)

        resp = CTX.socket(zmq.PUB)
        resp.connect(pub_addr)
        resp.setsockopt(zmq.IDENTITY, sender_id)

        self.sub_addr = sub_addr
        self.pub_addr = pub_addr
        self.reqs = reqs
        self.resp = resp


    def recv(self):
        """
        Receives a raw mongrel2.handler.Request object that you
        can then work with.
        """
        return Request.parse(self.reqs.recv())

    def recv_json(self):
        """
        Same as regular recv, but assumes the body is JSON and 
        creates a new attribute named req.data with the decoded
        payload.  This will throw an error if it is not JSON.

        Normally Request just does this if the METHOD is 'JSON'
        but you can use this to force it for say HTTP requests.
        """
        req = self.recv()

        if not req.data:
            req.data = json.loads(req.body)

        return req

    def send(self, uuid, conn_id, msg):
        """
        Raw send to the given connection ID at the given uuid, mostly used 
        internally.
        """
        header = "%s %d:%s," % (uuid, len(str(conn_id)), str(conn_id))
        self.resp.send(header + ' ' + msg)


    def reply(self, req, msg):
        """
        Does a reply based on the given Request object and message.
        This is easier since the req object contains all the info
        needed to do the proper reply addressing.
        """
        self.send(req.sender, req.conn_id, msg)


    def reply_json(self, req, data):
        """
        Same as reply, but tries to convert data to JSON first.
        """
        self.send(req.sender, req.conn_id, json.dumps(data))


    def reply_http(self, req, body, code=200, status="OK", headers=None):
        """
        Basic HTTP response mechanism which will take your body,
        any headers you've made, and encode them so that the 
        browser gets them.
        """
        self.reply(req, http_response(body, code, status, headers or {}))


    def deliver(self, uuid, idents, data):
        """
        This lets you send a single message to many currently
        connected clients.  There's a MAX_IDENTS that you should
        not exceed, so chunk your targets as needed.  Each target
        will receive the message once by Mongrel2, but you don't have
        to loop which cuts down on reply volume.
        """
        self.send(uuid, ' '.join(idents), data)


    def deliver_json(self, uuid, idents, data):
        """
        Same as deliver, but converts to JSON first.
        """
        self.deliver(uuid, idents, json.dumps(data))


    def deliver_http(self, uuid, idents, body, code=200, status="OK", headers=None):
        """
        Same as deliver, but builds an HTTP response, which means, yes,
        you can reply to multiple connected clients waiting for an HTTP 
        response from one handler.  Kinda cool.
        """
        self.deliver(uuid, idents, http_response(body, code, status, headers or {}))


