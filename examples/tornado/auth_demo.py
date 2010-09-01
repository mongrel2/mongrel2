from mongrel2.config import *


main = Server(
    uuid="f400bf85-4538-4f7a-8908-67e313d515c2",
    access_log="/logs/access.log",
    error_log="/logs/error.log",
    chroot="./",
    default_host="localhost",
    name="test",
    pid_file="/run/mongrel2.pid",
    port=6767,
    hosts = [
        Host(name="localhost", 
             routes={ r'/(.*)': Proxy(addr='127.0.0.1', port=8888) })
    ]
)

commit([main])


