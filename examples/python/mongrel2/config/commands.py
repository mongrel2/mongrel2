from mongrel2 import config
from mongrel2.config import args
import mongrel2.config.commands
from uuid import uuid4
from mongrel2.config import model
import getpass
import sys
import os
import signal
from sqlite3 import OperationalError


def try_reading(reader):
    try:
        cmd = reader.readline()
        return cmd.split(' ')

    except UnicodeDecodeError:
        print "\nERROR: Sorry, PyRepl and Python hate printing to your screen: UnicodeDecodeError."

    return []


def shell_command():
    """
    Starts an interactive shell with readline style input so you can
    work with Mongrel2 easier.
    """
    try:
        from pyrepl.unix_console import UnixConsole
        from pyrepl.historical_reader import HistoricalReader
    except:
        print "You don't have PyRepl installed, shell not available."

    reader = HistoricalReader(UnixConsole())
    reader.ps1 = "m2> "
    reader.ps2 = "..> "
    reader.ps3 = "...> "
    reader.ps4 = "....> "

    try:
        while True:

            cmd = try_reading(reader)
            if cmd:
                try:
                    args.parse_and_run_command(cmd, mongrel2.config.commands)
                except Exception, e:
                    print "ERROR:", e

    except EOFError:
        print "Bye."
    except KeyboardInterrupt:
        print "BYE!"


def help_command(**options):
    """
    Prints out help for the commands. 

    m2sh help

    You can get help for one command with:

    m2sh help -for STR
    """
    if "for" in options:
        help_text = args.help_for_command(config.commands, options['for'])

        if help_text:
            print help_text
        else:
            args.invalid_command_message(config.commands)
    else:
        print "Available commands:\n"
        print "\n".join(args.available_commands(config.commands))
        print "\nUse config help -for <command> to find out more."


def dump_command(db=None):
    """
    Simple dump of a config database:

        m2sh dump -db config.sqlite
    """

    print "LOADING DB: ", db

    try:
        if not (os.path.isfile(db) and os.access(db, os.R_OK)):
            raise IOError
    
        store = model.begin(db)
        servers = store.find(model.Server)

        for server in servers:
            print server

            for host in server.hosts:
                print "\t", host

                for route in host.routes:
                    print "\t\t", route
    except IOError:
        print "%s not readable" % db
    except OperationalError, exc:
        print "SQLite error: %s" % exc


def uuid_command(hex=False):
    """
    Generates a UUID for you to use in your configurations:

        m2sh uuid
        m2sh uuid -hex

    The -hex means to print it as a big hex number, which is
    more efficient but harder to read.
    """
    if hex:
        print uuid4().hex
    else:
        print str(uuid4())


def servers_command(db=None):
    """
    Lists the servers that are configured in this setup:

        m2sh servers -db config.sqlite
    """
    if not os.path.isfile(db):
        print "ERROR: Cannot access database file %s" % db
        return

    try:
        store = model.begin(db)
        servers = store.find(model.Server)
        for server in servers:
            print "-------"
            print server.name, server.default_host, server.uuid

            for host in server.hosts:
                print "\t", host.id, ':', host.name

    except OperationalError, exc:
        print "SQLite error: %s" % exc


def hosts_command(db=None, uuid="", host="", name=""):
    """
    List all the hosts in the given server identified by UUID or host.

        m2sh hosts -db config.sqlite -uuid f400bf85-4538-4f7a-8908-67e313d515c2
        m2sh hosts -db config.sqlite -host localhost
        m2sh hosts -db config.sqlite -name test

    The -host parameter is the default_host for the server.
    """


    if not (os.path.isfile(db) and os.access(db, os.R_OK)):
        print "Cannot read database file %s" % db
        return

    try:
        store = model.begin(db)
        results = None

        if uuid:
            results = store.find(model.Server, model.Server.uuid == unicode(uuid))
        elif host:
            results = store.find(model.Server, model.Server.default_host == unicode(host))
        elif name:
            results = store.find(model.Server, model.Server.name == unicode(name))
        else:
            print "ERROR: Must give a -host or -uuid or -name."
            return

        if results.count():
            server = results[0]
            hosts = store.find(model.Host, model.Host.server_id == server.id)
            for host in hosts:
                print "--------"
                print host, ":"
                for route in host.routes:
                    print "\t", route.path, ':', route.target
            
        else:
            print "No servers found."
    except OperationalError, exc:
        print "SQLite error: %s" % exc


def init_command(db=None):
    """
    Initializes a new config database.

        m2sh init -db config.sqlite

    It will obliterate this config.
    """
    from pkg_resources import resource_stream
    import sqlite3

    sql = resource_stream('mongrel2', 'sql/config.sql').read()

    if model.store:
        model.store.close()
        model.store = None

    if os.path.isfile(db) and not os.access(db, os.W_OK):
        print "Cannot access database file %s" % db
        return

    try:
        conn = sqlite3.connect(db)
        conn.executescript(sql)
   
        commit_command(db=db, what="init_command", why=" ".join(sys.argv))
    except OperationalError, exc:
        print "Error: %s" % exc


def load_command(db=None, config=None, clear=True):
    """
    After using init you can use this to load a config:

        m2sh load -db config.sqlite -config tests/sample_conf.py 

    This will erase the previous config, but we'll make it
    safer later on.
    """
    import imp

    if not (os.path.isfile(db) and os.access(db, os.R_OK)):
        print "Cannot access database file %s" % db
        return

    try:

        model.begin(db, clear=clear)
        imp.load_source('mongrel2_config_main', config)

        commit_command(db=db, what="load_command", why=" ".join(sys.argv))
    except OperationalError, exc:
        print "SQLite error: %s" % exc
    except SyntaxError,exc:
        print "Syntax error: %s" % exc


def config_command(db=None, config=None, clear=True):
    """
    Effectively does an init then load of a config to get
    you started quicker:

        m2sh config -db config.sqlite -config tests/sample_conf.py

    Like the other two, this will nuke your config, but we'll
    make it safer later.
    """

    init_command(db=db)
    load_command(db=db, config=config, clear=clear)


def commit_command(db=None, what=None, why=None):
    """
    Used to a commit event to the database for other admins to know
    what is going on with the config.  The system logs quite a lot
    already for you, like your username, machine name, etc:

        m2sh commit -db test.sqlite -what mongrel2.org \
            -why "Needed to change paters."

    In future versions it will prevent you from committing as root,
    because only assholes commit from root.

    Both parameters are arbitrary, but I like to record what I did to
    different Hosts in servers.
    """
    import socket

    store = model.load_db("sqlite:" + db)
    who = unicode(getpass.getuser())

    if who == u'root':
        print "Commit from root eh?  Man, you're kind of a tool."

    log = model.Log()
    log.who = who
    log.what = unicode(what)
    log.why = unicode(why)
    log.location = unicode(socket.gethostname())
    log.how = u'm2sh'

    store.add(log)
    store.commit()
    

def log_command(db=None, count=20):
    """
    Dumps commit logs:

        m2sh log -db test.sqlite -count 20
        m2sh log -db test.sqlite

    So you know who to blame.
    """

    store = model.load_db("sqlite:" + db)
    logs = store.find(model.Log)

    for log in logs.order_by(model.Log.happened_at)[0:count]:
        print log


def find_servers(db=None, uuid="", host="", name="", every=False):
    """
    Finds all the servers which match the given uuid, host or name.
    If every is true all servers in the database will be returned.
    """
    store = model.begin(db)
    servers = []

    if every:
        servers = store.find(model.Server)
    elif uuid:
        servers = store.find(model.Server, model.Server.uuid == unicode(uuid))
    elif host:
        servers = store.find(model.Server, model.Server.default_host == unicode(host))
    elif name:
        servers = store.find(model.Server, model.Server.name == unicode(name))
    
    return servers


def start_command(db=None, uuid= "", host="", name="", sudo=False, every=False):
    """
    Does a simple start of the given server(s) identified by the uuid, host
    (default_host) parameter or the name.:


        m2sh start -db config.sqlite -uuid 3d815ade-9081-4c36-94dc-77a9b060b021
        m2sh start -db config.sqlite -host localhost
        m2sh start -db config.sqlite -name test
        m2sh start -db config.sqlite -every


    Give the -sudo options if you want it to start mongrel2 as root for you
    (must have sudo installed).

    Give the -every option if you want mongrel2 to launch all servers listed in
    the given db.

    Note when using the host or name to select servers, all servers matching
    will be started.
    """
    root_enabler = 'sudo' if sudo else ''

    servers = find_servers(db, uuid, host, name, every)

    if servers.count() == 0:
        print 'No matching servers found, nothing launched'
    else:
        for server in servers:
            print 'Launching server %s %s on port %d' % (server.name, server.uuid, server.port)
            os.system('%s mongrel2 %s %s' % (root_enabler, db, server.uuid))


def stop_command(db=None, uuid="", host="", name="", every=False, murder=False):
    """
    Stops a running mongrel2 process according to the host, either
    gracefully (INT) or murderous (TERM):

        m2sh stop -db config.sqlite -host localhost
        m2sh stop -db config.sqlite -host localhost -murder
        m2sh stop -db config.sqlite -name test -murder
        m2sh stop -db config.sqlite -every

    You shouldn't need sudo to stop a running mongrel if you
    are also the user that owns the chroot directory or root.

    Normally mongrel2 will wait until connections die off before really
    leaving, but you can give it the -murder flag and it'll nuke it
    semi-gracefully.  You can also do it again with -murder if it's waiting
    for some dead connections and you want it to just quit.
    """
    for server in find_servers(db, uuid, host, name, every):
        pid = get_server_pid(server)
        if pid:
            sig = signal.SIGTERM if murder else signal.SIGINT
            os.kill(pid, sig)


def reload_command(db=None, uuid="", host="", name="", every=False):
    """
    Causes Mongrel2 to do a soft-reload which will re-read the config
    database and then attempt to load a whole new configuration without
    losing connections on the previous one:

        m2sh reload -db config.sqlite -uuid 3d815ade-9081-4c36-94dc-77a9b060b021
        m2sh reload -db config.sqlite -host localhost
        m2sh reload -db config.sqlite -name test
        m2sh reload -db config.sqlite -every

    This reload will need access to the config database from within the 
    chroot for it to work, and it's not totally guaranteed to be 100%
    reliable, but if you are doing development and need to do quick changes
    then this is what you do.
    """
    for server in find_servers(db, uuid, host, name, every):
        pid = get_server_pid(server)
        if pid:
            os.kill(pid, signal.SIGHUP)


def running_command(db=None, uuid="", host="", name="", every=False):
    """
    Tells you if the given server is still running:

        m2sh running -db config.sqlite -uuid 3d815ade-9081-4c36-94dc-77a9b060b021
        m2sh running -db config.sqlite -host localhost
        m2sh running -db config.sqlite -name test
        m2sh running -db config.sqlite -every
    """
    for server in find_servers(db, uuid, host, name, every):
        pid = get_server_pid(server)
        # TODO: Clean this up.
        if pid:
            try:
                os.kill(pid, 0)
                print "Found server %s %s RUNNING at PID %i" % (server.name,
                                                                server.uuid,
                                                                pid)
            except OSError:
                print "Server %s %s NOT RUNNING at PID %i" % (server.name,
                                                              server.uuid,
                                                              pid)


def control_command(db=None, host="", name="", uuid=""):
    """
    Start a simple control console for working with mongrel2.
    This is *very* bare bones at the moment but should improve.

        m2sh control -db config.sqlite -uuid 3d815ade-9081-4c36-94dc-77a9b060b021
        m2sh control -db config.sqlite -host localhost
        m2sh control -db config.sqlite -name test
    """
    store = model.load_db("sqlite:" + db)
    import zmq

    servers = find_servers(db, uuid, host, name, False)

    if servers.count() > 1:
        print "Not sure which server to run, here's a list:"
        print "NAME HOST UUID"
        for server in servers:
            print server.name, server.default_host, server.uuid
    else:
        CTX = zmq.Context()

        results = store.find(model.Setting, model.Setting.key == unicode("control_port"))
        addr = results[0].value if results.count() > 1 else "ipc://run/control"

        ctl = CTX.socket(zmq.REQ)

        print "CONNECTING..."
        ctl.connect(addr)

        try:
            while True:
                cmd = raw_input("> ")
                ctl.send(cmd)
                print ctl.recv()

        except EOFError:
            ctl.close()



def get_server_pid(server):
    pid_file = os.path.realpath(server.chroot + server.pid_file)
    if not os.path.isfile(pid_file):
        print "PID file %s not found for server %s %s" % (pid_file,
                                                          server.name,
                                                          server.uuid)
        return None
    else:
        return int(open(pid_file, 'r').read())

