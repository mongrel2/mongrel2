#!/usr/bin/python

from pprint import pprint

import subprocess
import os
import re
import sys

class GCovFileData:
    CallNotExecuted = 0
    CallReturned = 1
    BranchNotTaken = 0
    BranchTaken = 1
    BranchFallthrough = 2 # not used

    def __init__(self, keys, lines, calls, branches, functions):
        self.keys = keys
        self.lines = lines
        self.calls = calls
        self.branches = branches
        self.functions = functions

class GCDAData:
    def __init__(self, path, entries):
        self.path = path
        self.entries = entries
        
###
        
kGCovFileRE = re.compile('^File \'([^\n]*)\'$', re.DOTALL|re.MULTILINE)
kGCovFileAndOutputRE = re.compile('File \'([^\n]*)\'.*?:creating \'([^\n]*)\'',
                                  re.DOTALL|re.MULTILINE)

def parseGCovFile(gcovPath, baseDir):
    file = open(gcovPath)
    
    keys = {}
    lineData = []
    callData = []
    branchData = []
    functionData = []
    for ln in file:
        ln = ln[:-1]
        if ':' in ln:
            count,line,data = ln.split(':',2)
            line = int(line)
            count = count.strip()

            if count=='-':
                if line==0:
                    if ':' in data:
                        key,value = data.split(':',1)
                        if key in ('Source','Graph'):
                            value = os.path.normpath(os.path.join(baseDir,value))
                        keys[key] = value
                    else:
                        keys['Notes'] = keys.get('Notes','') + data + '\n'
                    continue
                else:
                    count = None
            elif count=='#####':
                count = 0
            else:
                count = int(count)

            if not lineData:
                if line!=1:
                    raise ValueError,'Unexpected start'
                lineData.append(count)
            else:
                for i in range(len(lineData),line-1):
                    lineData.append(None)
                if line != len(lineData)+1:
                    raise ValueError,'Unexpected line "%d"'%(line,)
                lineData.append(count)
        else:
            if ln.startswith('call '):
                ln = ln[4:].strip()
                num,data = ln.split(' ',1)
                num = int(num)
                if data=='never executed':
                    code = GCovFileData.CallNotExecuted
                    count = 0
                elif data.startswith('returned'):
                    code = GCovFileData.CallReturned
                    count = int(data[9:])
                else:
                    raise ValueError,'Unexpected call code: "%s"'%(data,)
                callData.append((len(lineData)-1,num,code,count))
            elif ln.startswith('branch '):
                ln = ln[6:].strip()
                num,data = ln.split(' ',1)
                num = int(num)
                if data=='never executed':
                    code = GCovFileData.BranchNotTaken
                elif data.startswith('taken '):
                    data = data[6:]
                    if ' ' in data:
                        count,extra = data.split(' ')
                        if extra=='(fallthrough)':
                            # I have no idea if we care about this
                            code = GCovFileData.BranchTaken
                        else:
                            raise ValueError,'Unexpected branch extra code: "%s"'%(data,)
                    else:
                        code = GCovFileData.BranchTaken
                        count = data
                    count = int(count)
                else:
                    raise ValueError,'Unexpected branch code: "%s"'%(data,)
                branchData.append((len(lineData)-1,num,code,count))
            elif ln.startswith('function '):
                ln = ln[9:].strip()
                name,data = ln.split(' ',1)
                if data.startswith('called '):
                    _,count,data = data.split(' ',2)
                    count = int(count)
                    # ignore other data
                else:
                    raise ValueError,'Unexpected function data: "%s"'%(data,)
                functionData.append((name,count))
            else:
                raise ValueError,'Unexpected stray line: "%s"'%(ln.strip())
    return GCovFileData(keys,lineData,callData,branchData,functionData)

def captureGCov(args):
    p = subprocess.Popen(args,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE)
    data,errdata = p.communicate()
    res = p.wait()
    if res:
        raise RuntimeError,'gcov failed: %s'%(res,)

    if errdata:
        print >>sys.stderr, 'WARNING: gcov produced errors'
        print >>sys.stderr, '\tCWD: %s'%(os.getcwd(),)
        print >>sys.stderr, '\tCommand: %s'%(' '.join(args),)
        print >>sys.stderr, '\tErrors: "%s"'%(errdata.replace('\n','\\n'),)
        
    return res,data

def getGCDAFiles(gcdaPath):
    res,data = captureGCov(['gcov','-n',gcdaPath,
                        '-o',os.path.dirname(gcdaPath)])
    entries = []    
    for res in kGCovFileRE.finditer(data):
        yield res.group(1)
        
def parseGCDA(gcdaPath,basePath=None):
    gcdaPath = os.path.realpath(gcdaPath)
    if basePath is None:
        basePath = os.path.dirname(gcdaPath)
    prevdir = os.getcwd()
    os.chdir(basePath)
    res,data = captureGCov(['gcov','-p','-l','-b','-c',
                            gcdaPath,
                            '-o', os.path.dirname(gcdaPath)])

    entries = []
    for res in kGCovFileAndOutputRE.finditer(data):
        path,output = res.groups()
        path = os.path.realpath(path)
        entries.append((path, parseGCovFile(output, basePath)))
        os.remove(output)

    os.chdir(prevdir)
    return GCDAData(os.path.realpath(gcdaPath), entries)

def main():
    from optparse import OptionParser
    op = OptionParser("usage: %prog [options] files")
    opts,args = op.parse_args()
    
    for f in args:
        print 'Parsing:',f
        res = parseGCDA(f)
        pprint(res)
        for p,e in res.entries:
            print p
        
if __name__=='__main__':
    main()
