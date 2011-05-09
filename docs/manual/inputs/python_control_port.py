import zmq
from mongrel2 import tnetstrings
from pprint import pprint

CTX = zmq.Context()

addr = "ipc://run/control"

ctl = CTX.socket(zmq.REQ)

print "CONNECTING"
ctl.connect(addr)

while True:
    cmd = raw_input("> ")
    # will only work with simple commands that have no arguments
    ctl.send(tnetstrings.dump([cmd, {}]))

    resp = ctl.recv()

    pprint(tnetstrings.parse(resp))

ctl.close()

