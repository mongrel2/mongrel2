try:
    import json
except:
    import simplejson as json

from mongrel2 import tnetstrings


class Request(object):

    def __init__(self, sender, conn_id, path, headers, body, msg=None):
        self.sender = sender
        self.path = path
        self.conn_id = conn_id
        self.headers = headers
        self.body = body
        self.msg = msg

        if self.headers['METHOD'] == 'JSON':
            self.data = json.loads(body)
        else:
            self.data = {}

    @staticmethod
    def parse(msg):
        sender, conn_id, path, rest = msg.split(' ', 3)
        headers, rest = tnetstrings.parse(rest)
        body, _ = tnetstrings.parse(rest)

        if type(headers) is str:
            headers = json.loads(headers)

        return Request(sender, conn_id, path, headers, body, msg=msg)

    def is_disconnect(self):
        if self.headers.get('METHOD') == 'JSON':
            return self.data['type'] == 'disconnect'

    def should_close(self):
        if self.headers.get('connection') == 'close':
            return True
        elif self.headers.get('VERSION') == 'HTTP/1.0':
            return True
        else:
            return False

    def forward(self, socket):
        """
        Forwards (and returns) the original raw message (if any) to a socket.
        This is useful for "chaining" handlers; the first handler can receive
        the message from Mongrel2, recognize it and pass the original raw
        message along to another handler.  The second handler's Connection
        connects to a socket in the first handler's (not Mongrel2).  Finally,
        the second (or subsequent) handler sends the response to Mongrel2.
        """
        if self.msg:
            socket.send( self.msg )
        return self.msg

    def encode(self):
        """
        Makes a raw message out of the Request object to be forwarded.
        """
        self.msg = " ".join([
            self.sender, self.conn_id, self.path,
            tnetstrings.dump(json.dumps(self.headers)) + tnetstrings.dump(self.body)
         ])
        return self.msg
