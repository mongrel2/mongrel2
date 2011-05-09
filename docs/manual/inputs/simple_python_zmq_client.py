import zmq

ctx = zmq.Context()
s = ctx.socket(zmq.SUB)
s.connect("tcp://127.0.0.1:5566")
s.setsockopt(zmq.SUBSCRIBE, '')

msg = s.recv()
print "MSG: ", repr(msg)

