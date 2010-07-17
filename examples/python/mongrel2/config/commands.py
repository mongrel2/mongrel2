from mongrel2 import config
from mongrel2.config import args
import mongrel2.config.commands


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
            cmd = reader.readline()
            if cmd:
                args.parse_and_run_command(
                    cmd.split(' '), mongrel2.config.commands,
                       default_command=None, exit_on_error=False)
    except EOFError:
        print "Bye."



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
            args.invalid_command_message(config.commands, exit_on_error=True)
    else:
        print "Available commands:\n"
        print ", ".join(args.available_commands(config.commands))
        print "\nUse config help -for <command> to find out more."



def dump_command(db=None):
    """
    Simple dump of a config database:

        m2sh dump -db config.sqlite
    """

    print "LOADING DB: ", db

    from mongrel2.config import model
    
    store = model.load_db("sqlite:" + db)
    servers = store.find(model.Server)

    for server in servers:
        print server

        for host in server.hosts:
            print "\t", host

            for route in host.routes:
                print "\t\t", route


def uuid_command(hex=False):
    """
    Generates a UUID for you to use in your configurations:

        m2sh uuid
        m2sh uuid -hex

    The -hex means to print it as a big hex number, which is
    more efficient but harder to read.
    """
    from uuid import uuid4

    if hex:
        print uuid4().hex
    else:
        print str(uuid4())

