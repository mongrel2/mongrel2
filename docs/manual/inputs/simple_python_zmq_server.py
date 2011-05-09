import zmq
import time

ctx = zmq.Context()
s = ctx.socket(zmq.PUB)
s.bind("tcp://127.0.0.1:5566")

while True:
    s.send("HELLO")
    time.sleep(1)

