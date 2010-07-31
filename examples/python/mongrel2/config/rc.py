import os
import ConfigParser

def read_rc():
    rcbasename = '.m2shrc'
    homedir = os.path.expanduser('~')
    rcfile = homedir + os.sep + rcbasename

    if not (os.path.isfile(rcfile) and os.access(rcfile, os.R_OK)):
        return

    config = ConfigParser.SafeConfigParser()
    config.read(rcfile)
    args = config.items('m2sh')

    dict = {}
    for k,v in args:
        dict[k] = v
    return dict
