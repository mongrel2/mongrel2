#!/usr/bin/python

from __future__ import division

from pprint import pprint

import cPickle
import os
import warnings

import GCovParser

class GCovGroup:
    @staticmethod
    def fromfile(path):
        f = open(path)
        try:
            res = cPickle.load(f)
            header,version = res[0],res[1]
        except:
            raise ValueError,'invalid zcov input'
        
        if header != 'zcov-data':
            raise ValueError,'input is not in zcov format'
        elif version != 1:
            raise ValueError,'unrecognized zcov version'

        return res[2]
    
    def tofile(self, path):
        f = open(path,'wb')
        cPickle.dump(('zcov-data',1,self),f,-1)
        f.close()
        
    def __init__(self):
        self.entryMap = {}

    def addEntry(self, path, entry):
        record = self.entryMap.get(path)
        if record is None:
            self.entryMap[path] = entry
        else:
            self.entryMap[path] = self.mergeData(record,entry)
        
    def addGCDA(self, data):
        for path,entry in data.entries:
            self.addEntry(path, entry)

    def merge(self, b):
        for path,entry in b.entryMap.items():
            self.addEntry(path, entry)

    def mergeData(self, a, b):
        keys = self.mergeKeys(a.keys, b.keys)
        lines = self.mergeLines(a.lines, b.lines)
        calls = self.mergeCalls(a.calls, b.calls)
        branches = self.mergeBranches(a.branches, b.branches)
        functions = self.mergeFunctions(a.functions, b.functions)
        return GCovParser.GCovFileData(keys, lines, calls, branches, functions)
    
    def mergeKeys(self, aKeys, bKeys):        
        if set(aKeys) != set(bKeys):
            raise ValueError,'Keys differ: %s, %s'%(pprint.pformat(a.keys),
                                                    pprint.pformat(b.keys))

        keys = {}
        for key,aValue in aKeys.items():
            bValue = bKeys[key]
            if key=='Source':
                if aValue != bValue:
                    raise ValueError,'Key ("%s") differs: %s %s'%(key,
                                                                  aValue,
                                                                  bValue)
                value = aValue
            elif key in ('Runs','Programs'):
                value = str(int(aValue) + int(bValue))
            elif key in ('Data','Graph'):
                value = aValue+','+bValue
            else:
                raise ValueError,'Unrecognized key: "%s"'%(key,)
            keys[key] = value
        return keys

    def mergeLines(self, aLines, bLines):
        if len(aLines) != len(bLines):
            raise ValueError,'Entry mismatch (number of lines)'

        lines = [None]*len(aLines)
        for i,(a,b) in enumerate(zip(aLines,bLines)):
            if a is None or b is None:
                # Executability can change across tests (conditional
                # code), take the non-None one if it exists.
                lines[i] = (a,b)[a is None]
            else:
                lines[i] = a + b
        return lines

    def mergeLineList(self, aList, bList, merge):
        if not aList:
            for bItem in bList:
                yield bItem
        elif not bList:
            for aItem in aList:
                yield aItem
        aIter,bIter = iter(aList),iter(bList)
        aItem,bItem = aIter.next(),bIter.next()
        while 1:
            if aItem[0]==bItem[0]:
                yield merge(aItem,bItem)
                try:
                    aItem = aIter.next()
                except StopIteration:
                    for bItem in bIter:
                        yield bItem
                    break
                try:
                    bItem = bIter.next()
                except StopIteration:
                    for aItem in aIter:
                        yield aItem
                    break
            elif aItem[0]<bItem[0]:
                yield aItem
                try:
                    aItem = aIter.next()
                except StopIteration:
                    yield bItem
                    for bItem in bIter:
                        yield bItem
                    break
            else:
                yield bItem
                try:
                    bItem = bIter.next()
                except StopIteration:
                    yield aItem
                    for aItem in bIter:
                        yield aItem
                    break
                
    def mergeCalls(self, aCalls, bCalls):
        def merge(a,b):
            if a[1] != b[1]:
                warnings.warn('Call mismatch (numbers differ)')
#                raise ValueError,'Call mismatch (numbers differ)'
            count = a[3]+b[3]
            code = GCovParser.GCovFileData.CallNotExecuted
            if GCovParser.GCovFileData.CallReturned in (a[2],b[2]):
                code = GCovParser.GCovFileData.CallReturned
            return (a[0],a[1],code,count)
        return list(self.mergeLineList(aCalls,bCalls,merge))

    def mergeBranches(self, aBranches, bBranches):
        def merge(a,b):
            # XXX This is really wrong
            if a[1] != b[1]:
                warnings.warn('Branch mismatch (numbers differ)')
#                raise ValueError,'Branch mismatch (numbers differ)'
            count = a[3]+b[3]
            code = GCovParser.GCovFileData.BranchNotTaken
            if GCovParser.GCovFileData.BranchTaken in (a[2],b[2]):
                code = GCovParser.GCovFileData.BranchTaken                
            return (a[0],a[1],code,count)
        return list(self.mergeLineList(aBranches,bBranches,merge))
    
    def mergeFunctions(self, aFunctions, bFunctions):
        def merge(a,b):
            if a[0] != b[0]:
                warnings.warn('Function mismatch (names differ)')
#                raise ValueError,'Function mismatch (names differ)'
            return (a[0],a[1]+b[1])
        return list(self.mergeLineList(aFunctions,bFunctions,merge))
    
###
        
def main():
    from optparse import OptionParser
    op = OptionParser("usage: %prog [options] files")
    opts,args = op.parse_args()

    group = GCovGroup()
    for f in args:
        res = GCovParser.parseGCDA(f)
        group.addGCDA(res)

    print '%d total files'%(len(group.entryMap),)

if __name__=='__main__':
    main()
