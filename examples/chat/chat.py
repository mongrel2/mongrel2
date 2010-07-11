import zmq
import simplejson as json

sender_id = "82209006-86FF-4982-B5EA-D1E29E55D481"


ctx = zmq.Context()
reqs = ctx.socket(zmq.SUB)
reqs.setsockopt(zmq.SUBSCRIBE, "")
reqs.connect("tcp://127.0.0.1:9999")

resp = ctx.socket(zmq.PUB)
resp.connect("tcp://127.0.0.1:9998")
resp.setsockopt(zmq.IDENTITY, sender_id)

users = {}
user_list = []


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

        print "SENDER:", sender, "ID:", conn_id, "PATH:", path, "HEADERS:", headers, "BODY:", repr(body)

        return Request(sender, conn_id, path, headers, body)


def send(i, data):
    d = i + ' ' + json.dumps(data)
    resp.send(d)


def deliver(data):
    # TODO: this will fail when it's over 100
    resp.send(' '.join(users.keys()) + ' ' + json.dumps(data))

while True:
    msg = reqs.recv()
    print "MESSAGE", repr(msg)

    req = Request.parse(msg)
    data = json.loads(req.body)

    try:
        if data["type"] == "join":
            deliver(data)
            users[req.conn_id] = data['user']
            user_list = [u[1] for u in users.items()]
            send(req.conn_id, {'type': 'userList', 'users': user_list})
        elif data["type"] == "leave":
            if req.conn_id in users:
                data['user'] = users[req.conn_id]
                del users[req.conn_id]

            deliver(data)
            user_list = [u[1] for u in users.items()]
        elif req.conn_id not in users:
            users[req.conn_id] = data['user']
        elif data['type'] == "msg":
            idiots = open("idiots").read()
            who = users.get(req.conn_id, "XXX")
            dwho = data.get('user', 'XXX')
            if who in idiots or dwho in idiots:
                print "IDIOT:", req.conn_id
                send(req.conn_id, data)
            else:
                deliver(data)

    except KeyError:
        print "BAD", data


    print "REGISTERED USERS:", len(users)

