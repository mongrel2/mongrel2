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
    resp.send(i + ' ' + json.dumps(data) + '\0')

def deliver(data):
    for i in users.keys():
        send(i, data)

while True:
    msg = reqs.recv()
    print "MESSAGE", repr(msg)

    ident, data = msg.split(' ', 1)
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
            deliver(data)
    except KeyError:
        print "BAD", data


    print "REGISTERED USERS:", len(users)

