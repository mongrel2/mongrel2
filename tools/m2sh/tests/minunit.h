#ifndef _minunit_h
#define _minunit_h

#include <stdio.h>
#include <dbg.h>
#include <stdlib.h>

#define mu_suite_start() char *message = NULL

/* file: minunit.h */
#define mu_assert(test, message) if (!(test)) { log_err(message); return message; }
#define mu_run_test(test) debug("\n-----%s", " " #test); message = test(); tests_run++; if (message) return message;

#define RUN_TESTS(name) void taskmain(int argc, char *argv[]) {\
     FILE *log_file = fopen("tests/tests.log", "a+");\
     if(!log_file) { printf("CAN'T OPEN TEST LOG\n"); exit(1); } \
     setbuf(log_file, NULL);\
     debug("----- RUNNING: %s", argv[0]);\
     printf("----\nRUNNING: %s\n", argv[0]);\
     char *result = name();\
     if (result != 0) {\
         printf("FAILED: %s\n", result);\
     }\
     else {\
         printf("ALL TESTS PASSED\n");\
     }\
     printf("Tests run: %d\n", tests_run);\
     exit(result != 0);\
 }


int tests_run;

#endif
