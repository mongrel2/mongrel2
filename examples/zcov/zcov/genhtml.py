from __future__ import division

from pprint import pprint

import os
import time
import subprocess
import sys

import GCovGroup
from GCovParser import GCovFileData

###

kBarWidth = 100

kCoverageClasses = [
    (40,  'low',  (255,  0,  0), '#FF9999'),
    (80,  'mid',  (255,255,  0), '#FFFF60'),
    (None,'high', (0,  255,  0), '#80FF99'),
    ]

kSummaryHeader = """\
<html>
<head>
  <script src="sorttable.js"></script>
  <script src="sourceview.js"></script>
  <link rel="stylesheet" type="text/css" href="style.css">
  <title>zcov: %(title)s</title>
</head>
<body>
<center><h1> %(linkedTitle)s </h1></center>
<hr>


<center>
<table id="headertable" cellpadding=2>
  <tr>
    <td bgcolor=%(overviewKeyBGColor)s> <b>Files:</b> </td>
    <td bgcolor=%(overviewValueBGColor)s> %(numEntries)d </td>
    <td width="30"></td>
    <td bgcolor=%(overviewKeyBGColor)s> <b>Branches&nbsp;Taken:</b> </td>
    <td bgcolor=%(overviewValueBGColor)s align=right> <b>%(totalPercentTakenBranches).1f%%<b> </td>
    <td bgcolor=%(overviewValueBGColor)s align=right> %(totalTakenBranches)d&nbsp;/&nbsp;%(totalTakeableBranches)d </td>
  </tr>
  <tr>
    <td bgcolor=%(overviewKeyBGColor)s> <b>Generated:</b> </td>
    <td bgcolor=%(overviewValueBGColor)s> %(generationTimestamp)s </td>
    <td width="30"></td>
    <td bgcolor=%(overviewKeyBGColor)s> <b>Branches&nbsp;Executed:</b> </td>
    <td bgcolor=%(overviewValueBGColor)s align=right> <b>%(totalPercentExecutedBranches).1f%%<b> </td>
    <td bgcolor=%(overviewValueBGColor)s align=right> %(totalExecutedBranches)d&nbsp;/&nbsp;%(totalExecutableBranches)d </td>
  </tr>
  <tr>
    <td colspan=2> </td>
    <td width="30"></td>
    <td bgcolor=%(overviewKeyBGColor)s> <b>Line&nbsp;Coverage:</b> </td>
    <td bgcolor=%(overviewValueBGColor)s align=right> <b>%(totalPercentLines).1f%%</b> </td>
    <td bgcolor=%(overviewValueBGColor)s align=right> %(totalCoveredLines)d&nbsp;/&nbsp;%(totalCoverableLines)d </td>
  </tr>
</table>
</center>
<p>
<hr>
"""
kFileHeader = """\
<center>
<table id="fileheadertable" cellpadding=2>
  <tr>
    <td bgcolor=%(overviewKeyBGColor)s> <b>Programs:</b> </td>
    <td bgcolor=%(overviewValueBGColor)s> %(numPrograms)s </td>
    <td width="30"></td>
    <td bgcolor=%(overviewKeyBGColor)s> <b>Runs</b> </td>
    <td bgcolor=%(overviewValueBGColor)s align=right> %(numRuns)s </td>
  </tr>
</table>
</center>
<p>
<hr>
"""

kSummaryFooter = """\
<hr>
Generated: %(generationTimestamp)s by <a href="http://minormatter.com/zcov">zcov</a><br>
</body>
</html>"""

kSummaryTableHeader = """\
<center>
<table id="summarytable" class="sortable" border="0" width="100%%">
  <thead>
  <tr>
    <th colspan=2 bgcolor=%(headerBGColor)s>
      <font size=+2 color="#FFFFFF"><u>Name</u></font> </th>
    <th colspan=4 bgcolor=%(headerBGColor)s sorttable_index=4 class="sorttable_numeric"> 
      <font size=+2 color="#FFFFFF"><u>Coverage</u></font> </th>
  </tr>
  <tr>
    <th colspan=2 bgcolor=%(header2BGColor)s class="sorttable_nosort"> </th>
    <th colspan=2 bgcolor=%(header2BGColor)s sorttable_index=2 class="sorttable_numeric">
      <font color="#FFFFFF"><u>Line</u></font> </th>
    <th colspan=2 bgcolor=%(header2BGColor)s sorttable_index=4 class="sorttable_numeric">
      <font color="#FFFFFF"><u>Branches Taken</u></font> </th>
  </tr>
  </thead>"""
kSummaryTableRow = """\
  <tr>
    <td bgcolor=%(rowBGColor)s>%(itemName)s</td>
    <td width=120 align=center bgcolor=%(rowBGColor)s>
      <div style="border:1px solid #000; background-color:#FFF; width:%(barTotalWidth)dpx;">
        <div style="background-color:%(classBGColor)s; width:%(barWidthLeft)dpx; height:10px;" width="3px"></div>
      </div>
    </td>
    <td width=90 align=right sorttable_customkey=%(lineKey)d bgcolor=%(classBGColor)s><b>%(percentLines).1f%%</b></td>
    <td width=1 align=right bgcolor=%(classBGColor)s>&nbsp;%(coveredLines)d&nbsp;/&nbsp;%(coverableLines)d&nbsp;lines</td>
    <td width=90 align=right sorttable_customkey=%(branchKey)d bgcolor=%(classBGColor)s><b>%(percentBranchesStr)s</b></td>
    <td width=1 align=right bgcolor=%(classBGColor)s>&nbsp;%(takenBranches)d&nbsp;/&nbsp;%(takeableBranches)d&nbsp;branches</td>
  </tr>"""
kSummaryTableFooter = """\
</table>
</center>"""

def safediv(a,b,default=None):
    try:
        return a/b
    except ZeroDivisionError:
        return default

def writeResources(directory):
    base = os.path.dirname(os.path.realpath(__file__))
    f = open(os.path.join(directory,'sorttable.js'),'w')
    f.write(open(os.path.join(base, 'js/sorttable.js')).read())
    f.close()
    f = open(os.path.join(directory,'sourceview.js'),'w')
    f.write(open(os.path.join(base, 'js/sourceview.js')).read())
    f.close()
    f = open(os.path.join(directory,'style.css'),'w')
    f.write(open(os.path.join(base, 'style.css')).read())
    f.close()

class GcovSummary:
    @staticmethod
    def fromfiledata(entry):
        be = bE = tb = tB = cl = cL = 0
        for line in entry.lines:
            if line is not None:
                cl += line != 0
                cL += 1
        for branch in entry.branches:
            if branch[2] != GCovFileData.BranchNotTaken:
                be += 1
            bE += 1
            
            tb += branch[3] != 0
            tB += 1
        return GcovSummary(1,tb,tB,be,bE,cl,cL)
    
    def __init__(self,
                 numFiles=0,
                 takenBranches=0, takeableBranches=0,
                 executedBranches=0, executableBranches=0,
                 coveredLines=0, coverableLines=0):
        self.numFiles = numFiles
        self.takenBranches = takenBranches
        self.takeableBranches = takeableBranches
        self.executedBranches = executedBranches
        self.executableBranches = executableBranches
        self.coveredLines = coveredLines
        self.coverableLines = coverableLines

    def __add__(self, b):
        return GcovSummary(self.numFiles + b.numFiles,
                           self.takenBranches + b.takenBranches,
                           self.takeableBranches + b.takeableBranches,
                           self.executedBranches + b.executedBranches,
                           self.executableBranches + b.executableBranches,
                           self.coveredLines + b.coveredLines,
                           self.coverableLines + b.coverableLines)

    def getBranchTakenPercent(self):
        return safediv(self.takenBranches,self.takeableBranches,0)
    def getBranchExecutedPercent(self):
        return safediv(self.executableBranches,self.executableBranches,0)
    def getLinePercent(self):
        return safediv(self.coveredLines,self.coverableLines,0)
    
class PathNode:
    def __init__(self, parent, elt):
        self.parent = parent
        self.elt = elt
        self.children = []
        self.item = None
        self.file = None
        self.summary = None
        self.covErrors = None
        self.itemsSummary = None
        
    def preorder(self):
        yield self
        for item in self.children:
            for x in item.preorder():
                yield x
                
    def postorder(self):
        for item in self.children:
            for x in item.postorder():
                yield x
        yield self

    def getPath(self):
        if self.parent is None:
            return self.elt.split('/')
        else:
            return self.parent.getPath() + self.elt.split('/')
        
    def getPathString(self):
        if self.elt is None:
            return 'TOP'
        elif self.elt is '':
            return '/'
        else:
            return '/'.join(self.getPath())

    def getNodeString(self):
        if self.elt:
            return self.elt
        elif self.elt is '':
            return '/'
        else:
            return 'TOP'

    def getSeparator(self):
        if self.elt is None:
            return ':'
        elif self.elt is '':
            return '&nbsp;'
        elif self.item is not None:
            return ''
        else:
            return '/'
        
    def getOrCreateChild(self, elt):
        for c in self.children:
            if c.elt == elt:
                return c
        self.children.append(PathNode(self,elt))
        return self.children[-1]

    def getStack(self):
        if self.parent is None:
            return [self]
        else:
            return self.parent.getStack() + [self]
        
def writeSummary(node, directory):
    path = os.path.join(directory, node.file)
    title = node.getPathString()

    elts = ''.join(['<a href="%s">%s</a>%s'%(s.file,s.getNodeString(),s.getSeparator())
                    for s in node.getStack()])
    linkedTitle = 'zcov: %s'%(elts)
    
    generationTimestamp = time.strftime('%Y-%m-%d %H:%M').replace(' ',
                                                                  '&nbsp;')

    totalTakenBranches = node.summary.takenBranches
    totalTakeableBranches = node.summary.takeableBranches
    totalExecutedBranches = node.summary.executedBranches
    totalExecutableBranches = node.summary.executableBranches
    totalCoveredLines = node.summary.coveredLines
    totalCoverableLines = node.summary.coverableLines
    totalPercentTakenBranches = 100.*safediv(totalTakenBranches,totalTakeableBranches,0)
    totalPercentExecutedBranches = 100.*safediv(totalExecutedBranches,totalExecutableBranches,0)
    totalPercentLines = 100.*safediv(totalCoveredLines,totalCoverableLines,0)
    numEntries = node.summary.numFiles

    items = [[item,0,0] for item in node.children]
    items.sort(key=lambda (e,_,__): (int(e.summary.getBranchTakenPercent()*1000),
                                  int(e.summary.getLinePercent()*1000),
                                  e.summary.takeableBranches,
                                  e.summary.coverableLines))
    for i,elt in enumerate(items):
        elt[1] = i
    items.sort(key=lambda (e,_,__): (int(e.summary.getLinePercent()*1000),
                                  e.summary.coverableLines,
                                  e.summary.takeableBranches))
    for i,elt in enumerate(items):
        elt[2] = i
    items.sort(key=lambda (e,_,__): e.elt)
    
    headerBGColor = '#5C5CEF'
    header2BGColor = '#ACACFF'
    rowBGColor = '#EEEEFF'

    overviewKeyBGColor = '#ACACFF'
    overviewValueBGColor = '#F0F0FF'

    f = open(path,'w')
    print >>f,kSummaryHeader%locals()
    if node.item is not None:
        path,entry = node.item

        if opts.root:
            path = opts.root + path

        numRuns = entry.keys.get('Runs')
        numPrograms = entry.keys.get('Programs')

        print >>f,kFileHeader%locals()
        if not os.path.exists(path):
            print >>sys.stderr,'WARNING: Unable to find source for "%s"'%(path,)
            print >>f,'Unable to find source'
        else:
            try:
                p = subprocess.Popen(['enscript','--highlight=c','--language=html',
                                      '--color', path, '-o', '-', '-q'],
                                     stdout=subprocess.PIPE)
            except OSError:
                raise ValueError,'enscript failed'
            data,errdata = p.communicate()
            res = p.wait()
            if res:
                raise ValueError,'enscript failed'

            lines = data.split('\n')
            start = end = None
            for i,ln in enumerate(lines):
                if ln=='<PRE>':
                    start = i + 1
                elif start and ln.endswith('</PRE>'):
                    end = i
            if not start or not end:
                raise ValueError,'enscript output unrecognized: "%s"'%(path,)
            if len(entry.lines) != end-start:
                # I have no idea why this happens, but it does.
                if len(entry.lines) != end - start + 1 or lines[-1]:
                    print >>sys.stderr,'WARNING: Line mismatch for "%s"'%(path,)
            print >>f,'<pre>'
            branchData = {}
            for b in entry.branches:
                lnIdx,num,code,count = b
                branchData[lnIdx] = branchData.get(lnIdx,[]) + [(num,code,count)]
            
            for i,ln in enumerate(lines[start:end]):
                if i in branchData:
                    f.write('<span class="branchGroup">')
                for (num,code,count) in branchData.get(i,()):
                    f.write('<span class="lineNum">         </span>')
                    if count:
                        print >>f, '<span class="branchTaken">%16d: branch %d taken</span>'%(count,num)
                    else:
                        print >>f, '<span class="branchNotTaken">%16d: branch %d not taken</span>'%(count,num)
                if i in branchData:
                    f.write('</span>')
                f.write('<span class="lineNum">%8d </span>'%(i+1,))
                if i<len(entry.lines):
                    count = entry.lines[i]
                else:
                    count = None
                if count is None:
                    f.write(' '*16+': ')
                elif not count:
                    f.write('<span class="lineNoCov">%16d: '%(count,))
                else:
                    f.write('<span class="lineCov">%16d: '%(count,))
                f.write(ln)
                if count is not None:
                    print >>f,'</span>'
                else:
                    print >>f
            print >>f,'</pre>'
    else:
        print >>f,kSummaryTableHeader%locals()
        for i,(item,branchKey,lineKey) in enumerate(items):
            itemName = '<a href="%s"> %s </a>'%(item.file,
                                                item.getNodeString())
            coverageBar = ''
            takenBranches = item.summary.takenBranches
            takeableBranches = item.summary.takeableBranches
            coveredLines = item.summary.coveredLines
            coverableLines = item.summary.coverableLines

            percentLines = 100.*safediv(coveredLines,coverableLines,0.)
            percentBranches = safediv(takenBranches,takeableBranches)
            if percentBranches is None:
                percentBranchesStr = 'N/A'
            else:
                percentBranchesStr = '%.1f%%'%(100.*percentBranches,)
            for cclass in kCoverageClasses:
                if cclass[0] is None or percentLines < cclass[0]:
                    break
            className = cclass[1]
            barTotalWidth = kBarWidth
            barWidthLeft = int(barTotalWidth * percentLines / 100.0)
            barWidthRight = barTotalWidth - barWidthLeft
            classBGColor = cclass[3]
            print >>f,kSummaryTableRow%locals()
        print >>f,kSummaryTableFooter%locals()
    
    print >>f,kSummaryFooter%locals()
    f.close()
    
def action_genhtml(name, args):
    """generate HTML report from coverage data files"""

    global opts
    from optparse import OptionParser
    op = OptionParser("usage: %%prog %s [options] input output" % (name,))
    op.add_option("", "--root", action="store", dest="root", default=None,
                  help="root directory to view files from")
    opts,args = op.parse_args(args)

    if len(args) != 2:
        op.error('invalid number of arguments')
    input,output = args

    try:
        group = GCovGroup.GCovGroup.fromfile(input)
    except ValueError,e:
        op.error(e)

    if not os.path.isdir(output):
        os.mkdir(output)

    outputItems = []

    root = PathNode(None,None)
    for path,entry in group.entryMap.items():
        if opts.root:
            if not path.startswith(opts.root):
                continue
            path = path[len(opts.root):]
            
        container = root
        for elt in path.split('/'):
            container = container.getOrCreateChild(elt)
        container.item = ([path,entry])

    # Flatten tree
    for c in root.preorder():
        # If we only have one (non-leaf) child just merge in.
        while len(c.children) == 1 and c.children[0].item is None:
            assert c.item is None
            
            kid, = c.children
            c.children = kid.children
            for grandkid in c.children:
                grandkid.parent = c
            if c.elt is None:
                c.elt = kid.elt
            else:
                c.elt = c.elt + '/' + kid.elt
        
    # Compute output paths
    pathKeys = set()
    for c in root.preorder():
        if c.parent is None:
            c.file = 'index.html'
        else:
            pathElts = c.getPath()
            for i in range(len(pathElts)):
                key = '_'.join(pathElts[-i-1:])
                key = key.replace('/','_')
                if not key:
                    key = '_'
                if key not in pathKeys:
                    pathKeys.add(key)
                    break
            else:
                raise ValueError,'path key generation failed'                
            c.file = key+'.html'
            
    # Compute summary stats
    for c in root.postorder():
        if c.item:
            s = GcovSummary.fromfiledata(c.item[1])
        else:
            s = GcovSummary()
        c.summary = sum([kid.summary for kid in c.children], s)
        
    for node in root.preorder():
        writeSummary(node, output)
        
    writeResources(output)
