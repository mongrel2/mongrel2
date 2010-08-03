from mp3stream import ConnectState, Streamer 
from mongrel2 import handler
import glob


sender_id = "9703b4dd-227a-45c4-b7a1-ef62d97962b2"

CONN = handler.Connection(sender_id, "tcp://127.0.0.1:9995",
                          "tcp://127.0.0.1:9994")

MP3_FILES = glob.glob("*.mp3")

print "PLAYING:", MP3_FILES

CHUNK_SIZE = 5 * 1024

STATE = ConnectState()

STREAMER = Streamer(MP3_FILES, STATE, CONN, CHUNK_SIZE, sender_id)
STREAMER.start()


while True:
    req = CONN.recv()

    if req.is_disconnect():
        print "DISCONNECT", req.headers, req.body, req.conn_id
        STATE.remove(req)
    else:
        print "REQUEST", req.headers, req.body

        if STATE.count() > 20:
            print "TOO MANY", STATE.count()
            CONN.reply_http(req, "Too Many Connected.  Try Later.")
        else:
            STATE.add(req)
            CONN.reply_http(req, "", headers={'icy-metaint': CHUNK_SIZE})


