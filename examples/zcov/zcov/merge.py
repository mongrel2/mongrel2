from __future__ import division

from pprint import pprint

import os

import GCovGroup

###

def action_merge(name, args):
    """merge multiple coverage data files"""

    from optparse import OptionParser
    op = OptionParser("usage: %%prog %s [options] output {inputs}" % (name,))
    op.add_option('-f','--force',
                  help='Force writing output',
                  dest='force', action='store_true', default=False)    
    opts,args = op.parse_args(args)
    
    if len(args) < 2:
        op.error('invalid number of arguments')
    output,inputs = args[0],args[1:]

    if not opts.force and os.path.exists(output):
        op.error('output exists, use -f to override')
    
    group = GCovGroup.GCovGroup()
    for input in inputs:
        try:
            inGroup = GCovGroup.GCovGroup.fromfile(input)
        except ValueError,e:
            op.error(e)
        group.merge(inGroup)
    group.tofile(output)
