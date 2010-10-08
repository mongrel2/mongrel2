from mongrel2 import handler
try:
    import json
except:
    import simplejson as json

sender_id = "82209006-86FF-4982-B5EA-D1E29E55D481"

conn = handler.Connection(sender_id, "tcp://127.0.0.1:9995",
                          "tcp://127.0.0.1:9994")

COUNT = 0

while True:
    print "WAITING FOR REQUEST", COUNT

    req = conn.recv()

    if req.is_disconnect():
        print "DISCONNECT"
        continue
    else:
        print "GOT ONE"
        COUNT += 1

