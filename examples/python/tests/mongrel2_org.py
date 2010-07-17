from mongrel2.config import *

begin("testconf.sqlite", clear=True)

main = Server(
    uuid="2f62bd5-9e59-49cd-993c-3b6013c28f05",
    access_log="/logs/access.log",
    error_log="/logs/error.log",
    chroot="/var/www/mongrel2",
    default_host="mongrel2.org",
    port=6767
)


test_directory = Dir(base='tests/',
                     index_file='index.html',
                     default_ctype='text/plain')

web_app_proxy = Proxy(addr='127.0.0.1', port=8080)

chat_demo_dir = Dir(base='examples/chat/static/', 
                    index_file='index.html', 
                    default_ctype='text/plain')

chat_demo = Handler(send_spec='tcp://127.0.0.1:9999',
                    send_ident='54c6755b-9628-40a4-9a2d-cc82a816345e',
                    recv_spec='tcp://127.0.0.1:9998', recv_ident='')

handler_test = Handler(send_spec='tcp://127.0.0.1:9997',
                       send_ident='54c6755b-9628-40a4-9a2d-cc82a816345e',
                       recv_spec='tcp://127.0.0.1:9996', recv_ident='')


mongrel2 = Host(name="mongrel2.org", routes={
    '@chat': chat_demo,
    '/handlertest': handler_test,
    '/chat/': web_app_proxy,
    '/': web_app_proxy,
    '/tests/': test_directory,
    '/testsmulti/(.*.json)': test_directory,
    '/chatdemo/': chat_demo_dir,
    '/static/': chat_demo_dir,
})

main.hosts = [mongrel2]

commit([main])


