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


# once you have this you get this segfault

==16310== Invalid write of size 1
==16310==    at 0x4E564EB: zmq::socket_base_t::close() (socket_base.cpp:451)
==16310==    by 0x4E5CE78: zmq_close (zmq.cpp:294)
==16310==    by 0x4035CB: close_handlers (mongrel2.c:247)
==16310==    by 0x405964: tst_traverse (tst.c:263)
==16310==    by 0x40364F: stop_handlers (mongrel2.c:258)
==16310==    by 0x403761: complete_shutdown (mongrel2.c:287)
==16310==    by 0x403BB3: taskmain (mongrel2.c:353)
==16310==    by 0x4116BF: taskmainstart (task.c:327)
==16310==    by 0x410D0C: taskstart (task.c:37)
==16310==    by 0x533DCAF: ??? (in /lib/libc-2.11.1.so)
==16310==    by 0x411143: contextswitch (task.c:180)
==16310==    by 0x645025F: ???
==16310==  Address 0xb8 is not stack'd, malloc'd or (recently) free'd
==16310== 
==16310== 
==16310== Process terminating with default action of signal 11 (SIGSEGV)
==16310==  Access not within mapped region at address 0xB8
==16310==    at 0x4E564EB: zmq::socket_base_t::close() (socket_base.cpp:451)
==16310==    by 0x4E5CE78: zmq_close (zmq.cpp:294)
==16310==    by 0x4035CB: close_handlers (mongrel2.c:247)
==16310==    by 0x405964: tst_traverse (tst.c:263)
==16310==    by 0x40364F: stop_handlers (mongrel2.c:258)
==16310==    by 0x403761: complete_shutdown (mongrel2.c:287)
==16310==    by 0x403BB3: taskmain (mongrel2.c:353)
==16310==    by 0x4116BF: taskmainstart (task.c:327)
==16310==    by 0x410D0C: taskstart (task.c:37)
==16310==    by 0x533DCAF: ??? (in /lib/libc-2.11.1.so)
==16310==    by 0x411143: contextswitch (task.c:180)

