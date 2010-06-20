#ifndef _minunit_h
#define _minunit_h

#include <stdio.h>
#include <dbg.h>

#define mu_suite_start() char *message = NULL

/* file: minunit.h */
#define mu_assert(test, message) if (!(test)) return message;
#define mu_run_test(test) message = test(); tests_run++; if (message) return message;

#define RUN_TESTS(name) int taskmain(int argc, char **argv) {\
     LOG_FILE = fopen("tests.log", "a+");\
     setbuf(LOG_FILE, NULL);\
     printf("----\nRUNNING: %s\n", argv[0]);\
     char *result = name();\
     if (result != 0) {\
         printf("%s\n", result);\
     }\
     else {\
         printf("ALL TESTS PASSED\n");\
     }\
     printf("Tests run: %d\n", tests_run);\
     return result != 0;\
 }


int tests_run;

#endif
