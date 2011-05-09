import zmq

ctx = zmq.Context()
s = ctx.socket(zmq.REQ)
s.connect("tcp://127.0.0.1:5566")

s.send('HI FROM CLIENT')

msg = s.recv()
print "MSG: ", repr(msg)

