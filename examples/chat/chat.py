import simplejson as json
from mongrel2 import handler

sender_id = "82209006-86FF-4982-B5EA-D1E29E55D481"

conn = handler.Connection(sender_id, "tcp://127.0.0.1:9999",
                          "tcp://127.0.0.1:9998")
users = {}
user_list = []


while True:
    try:
        req = conn.recv_json()
    except:
        print "FAILED RECV JSON"
        continue

    data = req.data

    print "DATA", data, req.conn_id

    if data["type"] == "join":
        conn.deliver_json(req.sender, users.keys(), data)
        users[req.conn_id] = data['user']
        user_list = [u[1] for u in users.items()]
        conn.reply_json(req, {'type': 'userList', 'users': user_list})

    elif data["type"] == "disconnect":
        print "DISCONNECTED", req.conn_id

        if req.conn_id in users:
            data['user'] = users[req.conn_id]
            del users[req.conn_id]

        if len(users.keys()) > 0:
            conn.deliver_json(req.sender, users.keys(), data)
            user_list = [u[1] for u in users.items()]

    elif req.conn_id not in users:
        users[req.conn_id] = data['user']

    elif data['type'] == "msg":
        conn.deliver_json(req.sender, users.keys(), data)

    print "REGISTERED USERS:", len(users)

