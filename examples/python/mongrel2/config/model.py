from storm.locals import *

database = None
store = None

TABLES = ["server", "host", "route", "proxy", "directory", "handler",
                  "setting"]

def load_db(spec):
    global database
    global store

    if not store:
        database = create_database(spec)
        store = Store(database)

    return store


def clear_db():
    for table in TABLES:
        store.execute("DELETE FROM %s" % table)


def begin(config_db, clear=False):
    store = load_db("sqlite:" + config_db)
    store.mongrel2_clear=clear

    if clear:
        clear_db()

    return store


def commit(servers, settings=None):
    for server in servers:
        store.add(server)

        for host in server.hosts:
            host.server = server
            store.add(host)


            for route in host.routes:
                route.host = host
                store.add(route)

    if store.mongrel2_clear:
        store.commit()
    else:
        print("Results won't be committed unless you begin(clear=True).")

    if settings:
        for k,v in list(settings.items()):
            store.add(Setting(str(k), str(v)))

        store.commit()


class Server(object):
    __storm_table__ = "server"
    id = Int(primary = True)
    uuid = Unicode()
    access_log = Unicode()
    error_log = Unicode()
    chroot = Unicode()
    default_host = Unicode()
    name = Unicode()
    pid_file = Unicode()
    port = Int()
    bind_addr = Unicode(default=str('0.0.0.0'))
    use_ssl = Bool(default = 0)

    def __init__(self, uuid=None, access_log=None, error_log=None,
                 chroot=None, default_host=None, name=None, pid_file=None,
                 port=None, hosts=None, bind_addr='0.0.0.0', use_ssl=False):
        super(Server, self).__init__()
        self.uuid = str(uuid)
        self.access_log = str(access_log)
        self.error_log = str(error_log)
        self.chroot = str(chroot)
        self.default_host = str(default_host)
        self.name = str(name) if name else self.default_host
        self.pid_file = str(pid_file)
        self.port = port
        self.bind_addr = str(bind_addr)
        self.use_ssl = use_ssl

        for h in hosts or []:
            self.hosts.add(h)

    def __repr__(self):
        return "Server(uuid=%r, access_log=%r, error_log=%r, chroot=%r, default_host=%r, port=%d)" % (
            self.uuid, self.access_log, self.error_log, 
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
        self.name = str(name)
        self.matching = matching or self.name
        self.maintenance = maintenance

        if routes:
            for p,t in list(routes.items()):
                self.routes.add(Route(path=p, target=t))


    def __repr__(self):
        return "Host(maintenance=%d, name=%r, matching=%r)" % (
            self.maintenance, self.name, self.matching)



Server.hosts = ReferenceSet(Server.id, Host.server_id)


class Handler(object):
    __storm_table__ = "handler"
    id = Int(primary = True)
    send_spec = Unicode()
    send_ident = Unicode()
    recv_spec = Unicode()
    recv_ident = Unicode()
    raw_payload = Bool(default = 0)
    protocol = Unicode(default = str('json'))

    def __init__(self, send_spec, send_ident, recv_spec, recv_ident,
                 raw_payload=False, protocol='json'):
        super(Handler, self).__init__()
        self.send_spec = str(send_spec)
        self.send_ident = str(send_ident)
        self.recv_spec = str(recv_spec)
        self.recv_ident = str(recv_ident)
        self.raw_payload = raw_payload
        self.protocol = str(protocol)

    def __repr__(self):
        return "Handler(send_spec=%r, send_ident=%r, recv_spec=%r, recv_ident=%r)" % (
            self.send_spec, self.send_ident, self.recv_spec,
            self.recv_ident)



class Proxy(object):
    __storm_table__ = "proxy"
    id = Int(primary = True)
    addr = Unicode()
    port = Int()

    def __init__(self, addr, port):
        super(Proxy, self).__init__()
        self.addr = str(addr)
        self.port = port
        

    def __repr__(self):
        return "Proxy(addr=%r, port=%d)" % (
            self.addr, self.port)



class Dir(object):
    __storm_table__ = "directory"
    id = Int(primary = True)
    base = Unicode()
    index_file = Unicode()
    default_ctype = Unicode()
    cache_ttl = Int(default=0)

    def __init__(self, base, index_file, default_ctype="text/plain", cache_ttl=0):
        super(Dir, self).__init__()
        self.base = str(base)
        self.index_file = str(index_file)
        self.default_ctype = str(default_ctype)
        self.cache_ttl = cache_ttl

    def __repr__(self):
        return "Dir(base=%r, index_file=%r, default_ctype=%r)" % (
            self.base, self.index_file, self.default_ctype)



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
        self.path = str(path)
        self.reversed = reversed
        self.host = host

        if target:
            store.add(target)
            store.commit()
            self.target_id = target.id
            self.target_type = str(target.__class__.__name__.lower())

    def target_class(self):
        return self._targets[self.target_type]

    @property
    def target(self):
        kls = self.target_class()
        targets = store.find(kls, kls.id == self.target_id)
        assert targets.count() <= 1, "Routes should only map to one target."
        return targets[0] if targets.count() else None

    def __repr__(self):
        return "Route(path=%r, reversed=%r, target=%r)" % (
            self.path, self.reversed, self.target)


Host.routes = ReferenceSet(Host.id, Route.host_id)

class Log(object):
    __storm_table__ = "log"
    id = Int(primary = True)
    who = Unicode()
    what = Unicode()
    happened_at = DateTime()
    location = Unicode()
    how = Unicode()
    why = Unicode()

    def __repr__(self):
        return "[%s, %s@%s, %s] %s" % (
            self.happened_at.isoformat(), self.who, self.location, self.what, 
            self.why)

class MIMEType(object):
    __storm_table__ = "mimetype"
    id = Int(primary = True)
    mimetype = Unicode()
    extension = Unicode()


    def __repr__(self):
        return "MIMEType(mimetype=%r, extension=%r)" % (
            self.mimetype, self.extension)


class Setting(object):
    __storm_table__ = "setting"
    id = Int(primary = True)
    key = Unicode()
    value = Unicode()

    def __init__(self, key, value):
        super(Setting, self).__init__()
        self.key = key
        self.value = value


    def __repr__(self):
        return "Setting(key=%r, value=%r)" % (self.key, self.value)

