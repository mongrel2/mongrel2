from mongrel2 import handler
import json
import glob
import time
import struct
from threading import Thread

sender_id = "9703b4dd-227a-45c4-b7a1-ef62d97962b2"

CONN = handler.Connection(sender_id, "tcp://127.0.0.1:9997",
                          "tcp://127.0.0.1:9996")

mp3s = glob.glob("*.mp3")

print "PLAYING:", mp3s

CHUNK_SIZE = 5 * 1024


CONNECTED={}

def make_icy_info(data):
    icy_info_len = len(data)
    assert icy_info_len % 16 == 0, "Must be multiple of 16."
    return struct.pack('B', icy_info_len / 16) + data


def stream_mp3(mp3_name):
    result = open(mp3_name, 'r')

    chunk = result.read(CHUNK_SIZE)
    icy_info = make_icy_info("StreamTitle='a';")
    empty_md = make_icy_info("")

    print CONNECTED.keys(), "ARE CONNECTED"

    # the first one has our little header for the song
    chunk = result.read(CHUNK_SIZE) + icy_info
    CONN.deliver(CONNECTED, chunk)

    # all of them after that are empty
    while chunk and CONNECTED:
        chunk = result.read(CHUNK_SIZE)

        if not chunk:
            break
        elif len(chunk) < CHUNK_SIZE:
            chunk += empty_md * (CHUNK_SIZE - len(chunk))

        CONN.deliver(CONNECTED.keys(), chunk + empty_md)
        time.sleep(0.2)


class Streamer(Thread):

    def run(self):
        while True:
            if CONNECTED:
                for mp3_name in mp3s:
                    print "Streaming %s" % mp3_name
                    stream_mp3(mp3_name)
                    time.sleep(10)
            else:
                time.sleep(0.1)

streamer = Streamer()
streamer.start()

while True:
    req = CONN.recv()

    if req.is_disconnect():
        print "DISCONNECT", req.headers, req.body, req.conn_id

        try:
            del CONNECTED[req.conn_id]
        except:
            print "NOT LISTED, WTF"
    else:
        print "REQUEST", req.headers, req.body
        CONNECTED[req.conn_id] = req
        CONN.reply_http(req, "", headers={'icy-metaint': CHUNK_SIZE})



