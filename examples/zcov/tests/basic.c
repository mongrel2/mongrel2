// Test basic 'zcov' operation.
//
// FIXME: It would be nice to separate the primary zcov operations from 'gcov',
// and then independently test the parsing functionality against different gcov
// versions and compilers.

// Create a sandbox directory.
//
// RUN: rm -rf %t.dir
// RUN: mkdir -p %t.dir
// RUN: cp %s %t.dir/basic.c

// Build the trivial test executable.
//
// RUN: /bin/sh -c 'cd %t.dir; cc -o t.exe basic.c -g --coverage'

// Run the executable.
//
// RUN: /bin/sh -c 'cd %t.dir; ./t.exe'

// Create the zcov data file.
//
// RUN: %{zcov} scan %t.dir/a.zcov %t.dir > %t.out
// RUN: grep "Found 1 .gcdas" %t.out

// Check the zcov summary info.
//
// RUN: %{zcov} summarize --root=%T %t.dir/a.zcov > %t.out
// RUN: FileCheck --check-prefix CHECK-SUMMARIZE < %t.out %s
//
// CHECK-SUMMARIZE: -- Total --
// CHECK-SUMMARIZE:   Files:  1
// CHECK-SUMMARIZE:   Lines   : 5/6: 83.33%
// CHECK-SUMMARIZE:   Branches: 0/0: 0.00%
// CHECK-SUMMARIZE: -- /basic.c.tmp.dir --
// CHECK-SUMMARIZE:   Files:  1
// CHECK-SUMMARIZE:   Lines   : 5/6: 83.33%
// CHECK-SUMMARIZE:   Branches: 0/0: 0.00%

// Remove the GCDA data and perform an additional run.
//
// RUN: rm %t.dir/basic.gcda
// RUN: /bin/sh -c 'cd %t.dir; ./t.exe extraarg'

// Create a new zcov data file (which should cover the other branch).
//
// RUN: %{zcov} scan %t.dir/b.zcov %t.dir

// Merge the two zcov data files, and check the summary.
//
// RUN: %{zcov} merge %t.dir/merged.zcov %t.dir/a.zcov %t.dir/b.zcov
// RUN: %{zcov} summarize --root=%T %t.dir/merged.zcov  > %t.out
// RUN: FileCheck --check-prefix CHECK-SUMMARIZE-MERGED < %t.out %s
//
// CHECK-SUMMARIZE-MERGED: -- Total --
// CHECK-SUMMARIZE-MERGED:   Files:  1
// CHECK-SUMMARIZE-MERGED:   Lines   : 6/6: 100.00%
// CHECK-SUMMARIZE-MERGED:   Branches: 0/0: 0.00%

// Check the zcov HTML output (just basic sanity at the moment).
//
// RUN: %{zcov} genhtml --root=%T %t.dir/merged.zcov %t.dir/html
// RUN: test -f %t.dir/html/index.html

#include <stdio.h>

int main(int argc, char **argv) {
  if (argc == 1) {
    printf("path A\n");
  } else {
    printf("path B\n");
  }
  return 0;
}



