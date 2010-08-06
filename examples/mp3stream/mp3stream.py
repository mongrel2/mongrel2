import time
import struct
from threading import Thread, Condition


class ConnectState(object):
    """
    Kind of a Queue but not.
    """

    def __init__(self):
        self.requests ={}
        self.wait_cond = Condition()

    def add(self, req):
        self.wait_cond.acquire()
        self.requests[req.conn_id] = req
        self.wait_cond.notify()
        self.wait_cond.release()


    def remove(self, req):
        self.wait_cond.acquire()
        try:
            del self.requests[req.conn_id]
        except:
            pass
        self.wait_cond.notify()
        self.wait_cond.release()

    def count(self):
        return len(self.requests)

    def available(self):
        self.wait_cond.acquire()
        self.wait_cond.wait()
        result = self.count()
        self.wait_cond.release()

        return result

    def connected(self):
        return self.requests.keys()



class Streamer(Thread):

    def __init__(self, mp3_files, state, conn, chunk_size, sender_id):
        super(Streamer, self).__init__()
        self.mp3_files = mp3_files
        self.state = state
        self.conn = conn
        self.chunk_size = chunk_size
        self.sender_id = sender_id


    def make_icy_info(self, data):
        icy_info_len = len(data)
        pad_len = (16 - (icy_info_len % 16)) % 16
        padding = struct.pack('B', 0) * pad_len
        return struct.pack('B', (icy_info_len + pad_len) / 16) + data + padding


    def stream_mp3(self, mp3_name):
        icy_info = self.make_icy_info("StreamTitle='%s';" % mp3_name)
        empty_md = self.make_icy_info("")

        print self.state.connected(), "Listeners connected."

        result = open(mp3_name, 'r')

        chunk = result.read(self.chunk_size)

        # the first one has our little header for the song
        self.conn.deliver(self.sender_id, self.state.connected(), chunk + icy_info)

        # all of them after that are empty
        while self.state.connected():
            chunk = result.read(self.chunk_size)

            if not chunk:
                break
            elif len(chunk) < self.chunk_size:
                chunk += empty_md * (self.chunk_size - len(chunk))

            self.conn.deliver(self.sender_id, self.state.connected(), chunk + empty_md)

            time.sleep(0.2)

        result.close()


    def run(self):
        print "RUNNING"
        while self.state.available():
            for mp3_name in self.mp3_files:
                print "Streaming %s" % mp3_name
                self.stream_mp3(mp3_name)
                if not self.state.connected():
                    break



