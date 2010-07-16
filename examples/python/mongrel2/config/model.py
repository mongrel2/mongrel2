from storm.locals import *

database = None
store = None

def load_db(spec):
    global database
    global store

    database = create_database(spec)
    store = Store(database)

    return store


class Server(object):
    __storm_table__ = "server"
    id = Int(primary = True)
    uuid = Unicode()
    access_log = Unicode()
    error_log = Unicode()
    chroot = Unicode()
    default_host = Unicode()
    port = Int()


    def __repr__(self):
        return "Server(id=%d, uuid=%r, access_log=%r, error_log=%r, chroot=%r, default_host=%r, port=%d)" % (
            self.id, self.uuid, self.access_log, self.error_log, 
            self.chroot, self.default_host, self.port)


class Host(object):
    __storm_table__ = "host"
    id = Int(primary = True)
    server_id = Int()
    server = Reference(server_id, Server.id)
    maintenance = Bool(default = 0)
    name = Unicode()
    matching = Unicode()


    def __repr__(self):
        return "Host(id=%d, server_id=%d, maintenance=%d, name=%r, matching=%r)" % (
            self.id, self.server_id, self.maintenance, self.name, self.matching)



Server.hosts = ReferenceSet(Server.id, Host.server_id)


class Handler(object):
    __storm_table__ = "handler"
    id = Int(primary = True)
    send_spec = Unicode()
    send_ident = Unicode()
    recv_spec = Unicode()
    recv_ident = Unicode()

    def __repr__(self):
        return "Handler(id=%d, send_spec=%r, send_ident=%r, recv_spec=%r, recv_ident=%r" % (
            self.id, self.send_spec, self.send_ident, self.recv_spec,
            self.recv_ident)



class Proxy(object):
    __storm_table__ = "proxy"
    id = Int(primary = True)
    addr = Unicode()
    port = Int()

    def __repr__(self):
        return "Proxy(id=%d, addr=%r, port=%d)" % (
            self.id, self.addr, self.port)

class Directory(object):
    __storm_table__ = "directory"
    id = Int(primary = True)
    base = Unicode()
    index_file = Unicode()
    default_ctype = Unicode()

    def __repr__(self):
        return "Dir(id=%d, base=%r, index_file=%r, default_ctype=%r)" % (
            self.id, self.base, self.index_file, self.default_ctype)


class Route(object):
    __storm_table__ = "route"
    id = Int(primary = True)
    path = Unicode()
    reversed = Bool(default = 0)
    host_id = Int()
    host = Reference(host_id, Host.id)
    target_id = Int()
    target_type = Unicode()

    _targets = {'dir': Directory,
                'handler': Handler,
                'proxy': Proxy}

    def target_class(self):
        return self._targets[self.target_type]

    @property
    def target(self):
        kls = self.target_class()
        targets = store.find(kls, kls.id == self.target_id)
        assert targets.count() <= 1, "Routes should only map to one target."
        return targets[0]

    def __repr__(self):
        return "Route(id=%d, path=%r, reversed=%r, host_id=%d, target=%r)" % (
            self.id, self.path, self.reversed, self.host_id, 
            self.target)


Host.routes = ReferenceSet(Host.id, Route.host_id)


class MIMEType(object):
    __storm_table__ = "mimetype"
    id = Int(primary = True)
    mimetype = Unicode()
    extension = Unicode()


    def __repr__(self):
        return "MIMEType(id=%d, mimetype=%r, extension=%r)" % (
            self.id, self.mimetype, self.extension)


