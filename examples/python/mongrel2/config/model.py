from storm.locals import *

class Server(object):
    __storm_table__ = "server"
    id = Int(primary = True)
    uuid = Unicode()
    access_log = Unicode()
    error_log = Unicode()
    chroot = Unicode()
    default_host = Unicode()
    port = Int()


class Host(object):
    __storm_table__ = "host"
    id = Int(primary = True)
    server_id = Int()
    server = Reference(server_id, Server.id)
    maintenance = Bool(default = 0)
    name = Unicode()
    matching = Unicode()

Server.hosts = ReferenceSet(Server.id, Host.server_id)

class Route(object):
    __storm_table__ = "route"
    id = Int(primary = True)
    path = Unicode()
    reversed = Bool(default = 0)
    host_id = Int()
    host = Reference(host_id, Host.id)
    target_id = Int()
    target_type = Unicode()


Host.routes = ReferenceSet(Host.id, Route.host_id)


class Handler(object):
    __storm_table__ = "handler"
    id = Int(primary = True)
    send_spec = Unicode()
    send_ident = Unicode()
    recv_spec = Unicode()
    recv_ident = Unicode()


class Proxy(object):
    __storm_table__ = "proxy"
    id = Int(primary = True)
    addr = Unicode()
    port = Int()

class Directory(object):
    __storm_table__ = "directory"
    id = Int(primary = True)
    base = Unicode()
    index_file = Unicode()
    default_ctype = Unicode()


class MIMEType(object):
    __storm_table__ = "mimetype"
    id = Int(primary = True)
    mimetype = Unicode()
    extension = Unicode()


database = create_database("sqlite:config.sqlite")
store = Store(database)


