from mongrel2.handler import CTX
from mongrel2 import tnetstrings
import zmq

class ControlPort(object):

    def __init__(self, addr):
        self.sock = zmq.Socket(CTX, zmq.REQ)
        self.sock.connect(addr)

    def request(self, name, **args):
        msg = tnetstrings.dump([name, args])
        self.sock.send(msg)
        rep = self.sock.recv()
        result, remain = tnetstrings.parse(rep)
        return result

