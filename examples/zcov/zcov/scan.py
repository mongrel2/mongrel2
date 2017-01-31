from __future__ import division

from pprint import pprint

import cPickle
import re
import os
import sys

import GCovParser, GCovGroup


###

kSourceFileRE = re.compile(r'.*\.(c|cpp|cc)$')

oldMethod="""
    # Scan inputs for source files and for .gcdas
    sources = set()
    gcdas = set()
    sourceFileMap = {}
    for dir in dirs:
        if not os.path.isdir(dir):
            op.error('Not a directory: "%s"'%(dir,))
        for path,dirnames,filenames in os.walk(os.path.abspath(dir)):
            for file in filenames:
                if kSourceFileRE.match(file):
                    filepath = os.path.join(path,file)
                    sources.add(filepath)
                    base,_ = os.path.splitext(file)
                    sourceFileMap[base] = sourceFileMap.get(base,[]) + [filepath]
                elif file.endswith('.gcda'):
                    gcdas.add(os.path.join(path,file))

    print 'Found - %d sources, %d gcdas'%(len(sources),len(gcdas))
    matches = []
    for f in gcdas:
        name,_ = os.path.splitext(os.path.basename(f))
        sourceFiles = sourceFileMap.get(name)
        if sourceFiles is None:
            # Find files referenced from .gcda
            gcdaFiles = GCovParser.getGCDAFiles(f)
            
            # Find source files which match any unprefixed entries
            gcdaSourceFiles = []
            for gf in gcdaFiles:
                if os.path.basename(gf)==gf:
                    base,_ = os.path.splitext(gf)
                    gcdaSourceFiles.extend(sourceFileMap.get(base,[]))

            # If result is unambiguous, use it
            if gcdaSourceFiles and len(gcdaSourceFiles)==1:
                matches.append((f,gcdaSourceFiles[0]))
            else:
                print '  No source for:',name
                matches.append((name,None))
        elif len(sourceFiles) > 1:
            # Look for one in same directory
            dir = os.path.dirname(f)
            sameDir = [sf for sf in sourceFiles if os.path.dirname(sf)==dir]
            if sameDir and len(sameDir)==1:
                matches.append((f,sameDir[0]))
            else:
                print '  Multiple sources for:',name
                matches.append((name,None))
        else:
            matches.append((f,sourceFiles[0]))
        
    group = GCovGroup.GCovGroup()
    for gf,sf in matches:
        res = GCovParser.parseGCDA(gf,sf)
        group.addGCDA(res)
"""

def findGCDABasePath(gf, sourcedirs):
    files = list(GCovParser.getGCDAFiles(gf))

    # First try relative to the gcda file
    basepath = os.path.dirname(os.path.abspath(gf))
    for f in files:
        if not os.path.exists(os.path.join(basepath,f)):
            # If this is absolute we are just hosed.
            if os.path.isabs(f):
                return None
            break
    else:
        return basepath

    # If user requested us to also look one up, then do so.
    if opts.lookUpDirs:
        for n in range(opts.lookUpDirs):
            basepath = os.path.dirname(basepath)
        for f in files:
            if not os.path.exists(os.path.join(basepath,f)):
                break
        else:
            return basepath
            
    # Collect all relative paths
    missing = [f for f in files if not os.path.isabs(f)]
    
    # See if we can resolve all missing files in some source
    # directory.
    okDirs = []
    for d in sourcedirs:
        for f in missing:
            if not os.path.exists(os.path.join(d,f)):
                break
        else:
            okDirs.append(d)

    # If we found exactly one match, use it.
    if len(okDirs)==1:
        return okDirs[0]

    # Otherwise pick the path with the longest shared prefix with the
    # gcda, under the assumption that the gcda is probably sitting
    # relative to the file. This scheme isn't good enough for
    # completely out of tree builds, but works for systems which drop
    # thing in a sub-directory of the source.
    best = None
    bestLen = 0
    for d in okDirs:
        pfLen = 0
        for a,b in zip(d, gf):
            if a != b: break
            pfLen += 1
        if best is None or pfLen > bestLen:
            best,bestLen = d,pfLen

    if best:
        print >>sys.stderr,'WARNING: Unable to find an unambiguous base path for files: "%s"' % ', '.join(missing)
        print >>sys.stderr,'   searching for: "%s"' % gf
        print >>sys.stderr,'           using: "%s" (based on prefix)' % best
        return best

    print >>sys.stderr,'WARNING: Unable to find a base path for files: "%s"'%(', '.join(missing),)

def action_scan(name, args):
    """create a coverage data file from gcov data"""

    from optparse import OptionParser
    global opts
    op = OptionParser("usage: %%prog %s [options] output {directories}" % (name,))
    op.add_option("", "--look-up-dirs",
                  action="store", dest="lookUpDirs", default=0, type=int,
                  metavar="N", help="when finding base paths, look up N directories")
    opts,args = op.parse_args(args)

    if not args:
        op.error('invalid number of arguments')
    output,dirs = args[0],args[1:]

    if not dirs:
        dirs = ['.']
        
    # Scan inputs for source files and for .gcdas
    sources = set()
    sourcedirs = set()
    gcdas = set()
    sourceFileMap = {}
    for dir in dirs:
        if not os.path.isdir(dir):
            op.error('Not a directory: "%s"'%(dir,))
        for path,dirnames,filenames in os.walk(os.path.abspath(dir)):
            sourcedirs.add(os.path.join(dir,path))
            if '.svn' in dirnames:
                dirnames.remove('.svn')
            for file in filenames:
                if file.endswith('.gcda'):
                    gcdas.add(os.path.join(path,file))

    print 'Found %d .gcdas (%d source dirs)'%(len(gcdas,), len(sourcedirs))
#    for f in gcdas:
#        gcdaFiles = GCovParser.getGCDAFiles(f)
#        print f,list(gcdaFiles)

    group = GCovGroup.GCovGroup()
    for gf in gcdas:
        # Determine the right directory to run gcov from so it can
        # find the all files.
        basepath = findGCDABasePath(gf, sourcedirs)
        if basepath is None:
            print >>sys.stderr, 'WARNING: Unable to find a base for "%s", skipping'%(gf,)
            continue
        res = GCovParser.parseGCDA(gf,basepath)
        group.addGCDA(res)

    group.tofile(output)
