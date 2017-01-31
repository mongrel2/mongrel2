from __future__ import division

from pprint import pprint

import os

import GCovGroup

###

def safediv(a,b):
    try:
        return a/b
    except ZeroDivisionError:
        return 0

def sorted(l):
    l = list(l)
    l.sort()
    return l

def summarizeEntries(header, entries):
    covLines = execLines = 0
    covBranches = execBranches = 0
    for entry in entries:
        for line in entry.lines:
            if line is not None:
                covLines += line != 0
                execLines += 1
        for branch in entry.branches:
            covBranches += branch[3] != 0
            execBranches += 1
    print '-- %s --'%(header,)
    print '  Files: ',len(entries)
    digits = max(len(str(execLines)),len(str(execBranches)))
    print '  Lines   : %*d/%*d: %.2f%%'%(digits,covLines,
                                         digits,execLines,
                                         100.*safediv(covLines,execLines))
    print '  Branches: %*d/%*d: %.2f%%'%(digits,covBranches,
                                         digits,execBranches,
                                         100.*safediv(covBranches,execBranches))

def action_summarize(name, args):
    """print a command line summary for coverage data"""

    from optparse import OptionParser
    op = OptionParser("usage: %%prog %s [options] input" % (name,))
    op.add_option("", "--root",
                  action="store", dest="root", default=None,
                  help="root directory to view files from")
    opts,args = op.parse_args(args)

    if len(args) != 1:
        op.error('invalid number of arguments')
    input, = args

    try:
        group = GCovGroup.GCovGroup.fromfile(input)
    except ValueError,e:
        op.error(e)

    allEntries = []
    byDir = {}
    for path,entry in group.entryMap.items():
        dirname = os.path.dirname(path)
        if opts.root:
            if not dirname.startswith(opts.root):
                continue
            dirname = dirname[len(opts.root):]
        byDir[dirname] = byDir.get(dirname,[]) + [entry]
        allEntries.append(entry)
    summarizeEntries('Total', allEntries)
    for path,entries in sorted(byDir.items()):
        summarizeEntries(path,entries)
