from mongrel2.config.model import *
from uuid import uuid4

store = load_db("sqlite:testconf.sqlite")
clear_db()

main = Server(uuid=uuid4().hex,
                access_log="/logs/access.log",
                error_log="/logs/error.log",
                chroot="/var/www/mongrel2",
                default_host="mongrel2.org",
                port=6767,
                hosts = [
                    Host(name="mongrel2.org", routes = [
                        Route(path="/test", 
                              target=Proxy('localhost', 8080)),

                        Route(path="/chatdemo/",
                              target=Dir("tests/", "index.html")),

                        Route(path="/handlertest",
                              target=Handler("tcp://127.0.0.1:9998", "",
                                             "tcp://127.0.0.1:9999", ""))
                    ])
                ])

store.add(main)
store.commit()

