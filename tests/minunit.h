#include "../src/dbg.h"

/* file: minunit.h */
#define mu_assert(message, test) if (!(test)) return message;
#define mu_run_test(test) char *message = test(); tests_run++; if (message) return message;

#define RUN_TESTS(name) int main(int argc, char **argv) {\
     logs = fopen("logs/tests.logs", "a+");\
     setbuf(logs, NULL);\
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

