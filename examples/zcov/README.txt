zcov - A code coverage reporting tool
=====================================

zcov is wrapper around the basic facilities of gcov for generating pretty
summaries for entire code bases. It is similar to lcov with an emphasis on nicer
HTML output (including, for example, branch coverage), and a greatly simplified
command line interface which tries to work around deficiencies in the metadata
provided by gcov.


Overview
========
zcov uses '.zcov' files to store the information about a run, and provides
several tools for working with these files. Use --help to see more information
on the tools.

zcov-scan
--
Recursively scan a directory structure looking for .gcda files and
create a .zcov containing all the results. Usage:
  $ zcov-scan output.zcov  ~/public/coreutils-6.10.cov

zcov-genhtml
--
Create an HTML report from a .zcov file. Usage:
  $ zcov-genhtml output.zcov coverage-report/

zcov-genhtml will make the output directory and write HTML files and an
index.html for all the files in the .zcov. One useful option is '--root' which
tells zcov to ignore any files outside the given root directory, and not to
write that root directory in the output (for example, so your home directory
path doesn't show up in the report).

zcov-summarize
--
Print a short textual summary of a .zcov file, for testing and simple command
line uses. Usage:
  $ zcov-summarize output.zcov

zcov-merge
--
Merge multiple .zcov files into one. Usage:
  $ zcov-merge output.zcov input1.zcov input2.zcov ... inputN.zcov


License
=======
zcov is available under the BSD license. See LICENSE.txt.


Credits
=======
zcov is written and maintained by Daniel Dunbar <daniel@zuster.org>

The JavaScript sorttable implementation as written by Stuart Langridge and is
included under the X11 license.
