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

def send(i, data):
    d = i + ' ' + json.dumps(data)
    resp.send(d)


def deliver(data):
    # TODO: this will fail when it's over 100
    resp.send(' '.join(users.keys()) + ' ' + json.dumps(data))

while True:
    msg = reqs.recv()
    print "MESSAGE", repr(msg)

    ident, path, data = msg.split(' ', 2)
    try:
        data = json.loads(data)
    except:
        continue

    print "IDENT:", ident, "MSG:", data

    try:
        if data["type"] == "join":
            deliver(data)
            users[ident] = data['user']
            user_list = [u[1] for u in users.items()]
            send(ident, {'type': 'userList', 'users': user_list})
        elif data["type"] == "leave":
            if ident in users:
                data['user'] = users[ident]
                del users[ident]

            deliver(data)
            user_list = [u[1] for u in users.items()]
        elif ident not in users:
            users[ident] = data['user']
        elif data['type'] == "msg":
            idiots = open("idiots").read()

            if users.get(ident, "XXX") in idiots or data['user'] in idiots:
                print "IDIOT:", ident
                send(ident, data)
            else:
                deliver(data)

    except KeyError:
        print "BAD", data


    print "REGISTERED USERS:", len(users)

