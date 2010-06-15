import zmq
import uuid

sender_id = uuid.uuid4().urn.split(':')[-1].upper()
print "SENDER ID:", sender_id

ctx = zmq.Context()
resp = ctx.socket(zmq.SUB)
resp.setsockopt(zmq.SUBSCRIBE, "")
resp.connect("tcp://127.0.0.1:9999")

reqs = ctx.socket(zmq.PUB)
reqs.connect("tcp://127.0.0.1:9998")
reqs.setsockopt(zmq.IDENTITY, sender_id)

while True:
    msg = raw_input("> ")
    reqs.send(msg)
    msg = resp.recv()
    print "RESP", msg


