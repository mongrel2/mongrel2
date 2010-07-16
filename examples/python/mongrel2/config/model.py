from storm.locals import *

database = None
store = None

def load_db(spec):
    global database
    global store

    database = create_database(spec)
    store = Store(database)

    return store


def clear_db():
    for table in ["server", "host", "route", "proxy", "directory", "handler"]:
        store.execute("DELETE FROM %s" % table)


class Server(object):
    __storm_table__ = "server"
    id = Int(primary = True)
    uuid = Unicode()
    access_log = Unicode()
    error_log = Unicode()
    chroot = Unicode()
    default_host = Unicode()
    port = Int()

    def __init__(self, uuid=None, access_log=None, error_log=None,
                 chroot=None, default_host=None, port=None, hosts=None):
        super(Server, self).__init__()
        self.uuid = unicode(uuid)
        self.access_log = unicode(access_log)
        self.error_log = unicode(error_log)
        self.chroot = unicode(chroot)
        self.default_host = unicode(default_host)
        self.port = port
        self.hosts = hosts or []

        for host in hosts:
            host.server = self
            store.add(host)


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

    def __init__(self, server=None, name=None, matching=None,
                 maintenance=False, routes=None):
        super(Host, self).__init__()
        self.server = server
        self.name = unicode(name)
        self.matching = matching or self.name
        self.maintenance = maintenance
        self.routes = routes or []

        for route in routes:
            route.host = self
            store.add(route)


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

    def __init__(self, send_spec, send_ident, recv_spec, recv_ident):
        super(Handler, self).__init__()
        self.send_spec = unicode(send_spec)
        self.send_ident = unicode(send_ident)
        self.recv_spec = unicode(recv_spec)
        self.recv_ident = unicode(recv_ident)


    def __repr__(self):
        return "Handler(id=%d, send_spec=%r, send_ident=%r, recv_spec=%r, recv_ident=%r" % (
            self.id, self.send_spec, self.send_ident, self.recv_spec,
            self.recv_ident)



class Proxy(object):
    __storm_table__ = "proxy"
    id = Int(primary = True)
    addr = Unicode()
    port = Int()

    def __init__(self, addr, port):
        super(Proxy, self).__init__()
        self.addr = unicode(addr)
        self.port = port
        

    def __repr__(self):
        return "Proxy(id=%d, addr=%r, port=%d)" % (
            self.id, self.addr, self.port)



class Dir(object):
    __storm_table__ = "directory"
    id = Int(primary = True)
    base = Unicode()
    index_file = Unicode()
    default_ctype = Unicode()

    def __init__(self, base, index_file, default_ctype="text/plain"):
        super(Dir, self).__init__()
        self.base = unicode(base)
        self.index_file = unicode(index_file)
        self.default_ctype = unicode(default_ctype)

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

    _targets = {'dir': Dir,
                'handler': Handler,
                'proxy': Proxy}

    def __init__(self, path=None, reversed=False, host=None, target=None):
        super(Route, self).__init__()
        self.path = unicode(path)
        self.reversed = reversed
        self.host = host

        if target:
            store.add(target)
            store.commit()
            self.target_id = target.id
            self.target_type = unicode(target.__class__.__name__.lower())

    def target_class(self):
        return self._targets[self.target_type]

    @property
    def target(self):
        kls = self.target_class()
        targets = store.find(kls, kls.id == self.target_id)
        assert targets.count() <= 1, "Routes should only map to one target."
        return targets[0] if targets.count() else None

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


