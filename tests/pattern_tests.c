#include "minunit.h"
#include <pattern.h>
#include <string.h>

FILE *LOG_FILE = NULL;

char *test_pattern_match() 
{
    const char *m = pattern_match("ZED", strlen("ZED"), "ZED");
    mu_assert(m != NULL, "Should match.");

    m = pattern_match("ZED", strlen("ZED"), "FOO");
    mu_assert(m == NULL, "Should not match.");

    m = pattern_match("ZEEEED", strlen("ZEEEED"), "ZE*D");
    mu_assert(m != NULL, "Should match.");

    m = pattern_match("ZEEEED", strlen("ZEEEED"), "ZX*D");
    mu_assert(m == NULL, "Should not match.");

    m = pattern_match("ZEEEED", strlen("ZEEEED"), "Z.*D");
    mu_assert(m != NULL, "Should match.");
    
    m = pattern_match("ZXXXXD", strlen("ZXXXXD"), "ZE.*D");
    mu_assert(m == NULL, "Should not match.");

    m = pattern_match("ZED", strlen("ZED"), ".*D$");
    mu_assert(m != NULL, "Should match.");

    m = pattern_match("ZED", strlen("ZED"), ".*X$");
    mu_assert(m == NULL, "Should not match.");

    // ( and ) are ignored but allowed for routing system later
    m = pattern_match("ZEED", strlen("ZEED"), "Z(.*D)$");
    mu_assert(m != NULL, "Should match.");

    // the frontier pattern, kinda weird, could be useful
    // used to find patterns between two other patterns
    m = pattern_match("THE (QUICK) brOWN FOx JUMPS",
             strlen("THE (QUICK) brOWN FOx JUMPS"), "%f[%a]%u-%f[%a]");
    mu_assert(m != NULL, "Should match.");

    // balanced string match, matches the {...} contents
    m = pattern_match("/users/{1234}", strlen("/users/{1234}"), "/users/%b{}");
    mu_assert(m != NULL, "Should match.");


    // char classes, which map to C functions:
    // %a' : isalpha(c)
    // %c' : iscntrl(c)
    // %d' : isdigit(c)
    // %l' : islower(c)
    // %p' : ispunct(c)
    // %s' : isspace(c)
    // %u' : isupper(c)
    // %w' : isalnum(c)
    // %x' : isxdigit(c)
    // %z' : (c == 0)  // NUL terminator
 
    m = pattern_match("a \t 9 l . \n U w A \0", 19, "%a %c %d %l %p %s %u %w %x %z");
    
    mu_assert(m != NULL, "Should match.");

    return NULL;
}


char * all_tests() {
    mu_suite_start();

    mu_run_test(test_pattern_match);

    return NULL;
}

RUN_TESTS(all_tests);


