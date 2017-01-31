import os
import sys

from zcov.genhtml import action_genhtml
from zcov.merge import action_merge
from zcov.scan import action_scan
from zcov.summarize import action_summarize

###

commands = dict((name[7:].replace("_","-"), f)
                for name,f in locals().items()
                if name.startswith('action_'))

def usage():
    print >>sys.stderr, "Usage: %s command [options]" % (
        os.path.basename(sys.argv[0]))
    print >>sys.stderr
    print >>sys.stderr, "Available commands:"
    cmds_width = max(map(len, commands))
    for name,func in sorted(commands.items()):
        print >>sys.stderr, "  %-*s - %s" % (cmds_width, name, func.__doc__)
    sys.exit(1)

def main():
    import sys

    if len(sys.argv) < 2 or sys.argv[1] not in commands:
        usage()

    cmd = sys.argv[1]
    commands[cmd](cmd, sys.argv[2:])

if __name__ == '__main__':
    main()
