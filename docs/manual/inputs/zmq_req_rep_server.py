import zmq

ctx = zmq.Context()
s = ctx.socket(zmq.REP)
s.bind("tcp://127.0.0.1:5566")

while True:
    print "GOT BACK", repr(s.recv())
    s.send("HELLO")
