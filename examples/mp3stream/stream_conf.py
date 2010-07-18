from mongrel2.config import *

main = Server(
    uuid="43d67166-15bc-4cb3-b727-be5ccf4eb8d1",
    access_log="/logs/access.log",
    error_log="/logs/error.log",
    chroot="./",
    pid_file="/run/mongrel2.pid",
    default_host="localhost",
    port=8787,
    hosts = [
        Host(name="mongrel2.org", routes={
            '/mp3stream': Handler(
                send_spec='tcp://127.0.0.1:9997',
                send_ident='53f9f1d1-1116-4751-b6ff-4fbe3e43d142',
                recv_spec='tcp://127.0.0.1:9996', recv_ident='')
        })
    ]
)

commit([main])


