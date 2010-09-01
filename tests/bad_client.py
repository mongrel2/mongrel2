import time
import socket


req = "GET /tests/sample.html HTTP/1.1\r\nHost: localhost\r\n"
leaker = "Accept: test\r\n"
ending = "\r\n" 

# req = '@chat {"type": "msg", "msg": "' + ("hello" * 500) + '}'

for i in range(0, 500):
    s = socket.socket()
    s.connect(("127.0.0.1", 6767))
    s.send(req)
    for x in range(0, 5000):
        s.send(leaker)
        time.sleep(0.2)
    s.send(ending)
    time.sleep(1)
    s.close()
