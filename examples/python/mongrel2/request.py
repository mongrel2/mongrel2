try:
    import json
except:
    import simplejson as json


def parse_netstring(ns):
    len, rest = ns.split(':', 1)
    len = int(len)
    assert rest[len] == ',', "Netstring did not end in ','"
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

        return Request(sender, conn_id, path, headers, body)


