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


test_directory = Dir(base='tests/',
                     index_file='index.html',
                     default_ctype='text/plain')

web_app_proxy = Proxy(addr='127.0.0.1', port=80)

chat_demo_dir = Dir(base='examples/chat/static/', 
                    index_file='index.html', 
                    default_ctype='text/plain')

chat_demo = Handler(send_spec='tcp://127.0.0.1:9999',
                    send_ident='54c6755b-9628-40a4-9a2d-cc82a816345e',
                    recv_spec='tcp://127.0.0.1:9998', recv_ident='')

handler_test = Handler(send_spec='tcp://127.0.0.1:9997',
                       send_ident='34f9ceee-cd52-4b7f-b197-88bf2f0ec378',
                       recv_spec='tcp://127.0.0.1:9996', recv_ident='')

# the r'' string syntax means to not interpret any \ chars, for regexes
mongrel2 = Host(name="mongrel2.org", routes={
    r'@chat': chat_demo,
    r'/handlertest': handler_test,
    r'/chat/': web_app_proxy,
    r'/': web_app_proxy,
    r'/tests/': test_directory,
    r'/testsmulti/(.*.json)': test_directory,
    r'/chatdemo/': chat_demo_dir,
    r'/static/': chat_demo_dir,
    r'/mp3stream': Handler(
        send_spec='tcp://127.0.0.1:9995',
        send_ident='53f9f1d1-1116-4751-b6ff-4fbe3e43d142',
        recv_spec='tcp://127.0.0.1:9994', recv_ident='')
})

main.hosts.add(mongrel2)

settings = {"zeromq.threads": 1}

commit([main], settings=settings)


