from mongrel2.config import *

main = Server(
    uuid="2f62bd5-9e59-49cd-993c-3b6013c28f05",
    access_log="/logs/access.log",
    error_log="/logs/error.log",
    chroot="./",
    pid_file="/run/mongrel2.pid",
    default_host="mongrel2.org",
    name="main",
    port=6767
)

handler_test1 = Handler(send_spec='tcp://127.0.0.1:9997',
                       send_ident='34f9ceee-cd52-4b7f-b197-88bf2f0ec378',
                       recv_spec='tcp://127.0.0.1:9996', recv_ident='')

main.hosts.add(
        Host(name="mongrel2.org", routes={
        r'/A': handler_test1,
        r'/B': handler_test1,
    }))

# second server has handler_test2, and even though you don't load this, it'll
# still create that handler anyway
backup = Server(
    uuid="2f62bd5-9e59-49cd-993c-3b6013c28f05",
    access_log="/logs/access.log",
    error_log="/logs/error.log",
    chroot="./",
    pid_file="/run/mongrel2.pid",
    default_host="localhost",
    name="backup",
    port=6767
)


handler_test2 = Handler(send_spec='tcp://127.0.0.1:9998',
                       send_ident='31329b4b-16df-4340-8186-9cccc0c1fd90',
                       recv_spec='tcp://127.0.0.1:9999', recv_ident='')

backup.hosts.add(
    Host(name="mongrel2.org", routes={
    r'/A': handler_test2,
    r'/B': handler_test2,
}))

commit([main, backup])

