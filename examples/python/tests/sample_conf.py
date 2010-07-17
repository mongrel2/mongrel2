from mongrel2.config import *

begin("testconf.sqlite")

main = Server(
    uuid="f400bf85-4538-4f7a-8908-67e313d515c2",
    access_log="/logs/access.log",
    error_log="/logs/error.log",
    chroot="./",
    default_host="localhost",
    port=6767,
    hosts = [
        Host(name="mongrel2.org", routes={
            '/tests/': Dir(base='tests/', index_file='index.html',
                             default_ctype='text/plain')
        })
    ]
)

commit([main])


