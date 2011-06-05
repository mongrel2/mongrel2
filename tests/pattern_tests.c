#include "minunit.h"
#include <pattern.h>
#include <string.h>

const char *m;

char *test_simple_matches()
{
    m = pattern_match("ZED", strlen("ZED"), "ZED");
    mu_assert(m != NULL, "Should match literal identically.");

    m = pattern_match("ZED", strlen("ZED"), "Z.D");
    mu_assert(m != NULL, "Should match wildcard.");

    m = pattern_match("ZED", strlen("ZED"), "FOO");
    mu_assert(m == NULL, "Should not match.");

    return NULL;
}

char *test_simple_repetitions()
{
    m = pattern_match("ZEEEED", strlen("ZEEEED"), "ZE*D");
    mu_assert(m != NULL, "Should match.");

    m = pattern_match("ZEEEED", strlen("ZEEEED"), "ZE*ED");
    mu_assert(m != NULL, "Should match.");

    m = pattern_match("ZED", strlen("ZEEEED"), "ZE*ED");
    mu_assert(m != NULL, "Should match.");

    m = pattern_match("ZD", strlen("ZD"), "ZE*D");
    mu_assert(m != NULL, "Should match.");

    m = pattern_match("ZEEEED", strlen("ZEEEED"), "ZX*D");
    mu_assert(m == NULL, "Should not match.");

    m = pattern_match("ZEEEED", strlen("ZEEEED"), "Z.*D");
    mu_assert(m != NULL, "Should match.");

    m = pattern_match("ZXXXXD", strlen("ZXXXXD"), "ZE.*D");
    mu_assert(m == NULL, "Should not match.");

    m = pattern_match("ZEEEED", strlen("ZEEEED"), "ZE+D");
    mu_assert(m != NULL, "Should match.");

    m = pattern_match("ZD", strlen("ZEEEED"), "ZE+D");
    mu_assert(m == NULL, "Should not match.");

    m = pattern_match("ZEEEED", strlen("ZEEEED"), "ZE-D");
    mu_assert(m != NULL, "Should match.");

    m = pattern_match("ZD", strlen("ZEEEED"), "ZE-D");
    mu_assert(m != NULL, "Should match.");

    return NULL;
}

char *test_anchors()
{
    m = pattern_match("ZED", strlen("ZED"), "ZED$");
    mu_assert(m != NULL, "Should match. Anchor at the end.");

    m = pattern_match("ZED", strlen("ZED"), "ZE$");
    mu_assert(m == NULL, "Should not match. Should have ended at $.'");

    m = pattern_match("ZED", strlen("ZED"), "ED");
    mu_assert(m == NULL, "Should not match. Implicit anchor at the start of pattern.");

    return NULL;
}

char *test_blocks()
{
    // ( and ) are ignored but allowed for routing system later
    m = pattern_match("ZED", strlen("ZEED"), "Z(ED)");
    mu_assert(m != NULL, "Should match.");

    bstring s = bfromcstr("ZED");
    bstring p = bfromcstr("Z(ED)");
    m = bstring_match(s, p);
    mu_assert(m != NULL, "Should match.");

    bdestroy(s); bdestroy(p);
    return NULL;
}

char *test_frontier()
{
    // the frontier pattern, kinda weird, could be useful
    // used to find patterns between two other patterns
    m = pattern_match("THE (QUICK) brOWN FOx JUMPS",
             strlen("THE (QUICK) brOWN FOx JUMPS"), "\\f[\\a]\\u-\\f[\\a]");
    mu_assert(m != NULL, "Should match.");

    return NULL;
}

char *test_balance()
{
    // balanced string match, matches the {...} contents
    m = pattern_match("/users/{1234}", strlen("/users/{1234}"), "/users/\\b{}");
    mu_assert(m != NULL, "Should match balanced chars.");

    m = pattern_match("/users/{1234", strlen("/users/{1234}"), "/users/\\b{}");
    mu_assert(m == NULL, "Should not match balanced unchars.");

    m = pattern_match("/users/1234}", strlen("/users/{1234}"), "/users/\\b{}");
    mu_assert(m == NULL, "Should not match balanced unchars.");

    return NULL;
}

char *test_set()
{
    m = pattern_match("ZED", strlen("ZED"), "Z[OE]D");
    mu_assert(m != NULL, "Should match.");

    m = pattern_match("ZED", strlen("ZED"), "Z[^OE]D");
    mu_assert(m == NULL, "Should not match.");

    m = pattern_match("ZED", strlen("ZED"), "Z[A-Z]D");
    mu_assert(m != NULL, "Should match.");

    return NULL;
}

char *test_optional()
{
    m = pattern_match("ZED", strlen("ZED"), "ZEE?D");
    mu_assert(m != NULL, "Should match.");

    m = pattern_match("ZEED", strlen("ZEED"), "ZEE?D");
    mu_assert(m != NULL, "Should match wildcard.");

    m = pattern_match("ZEEED", strlen("ZEEED"), "ZEE?D");
    mu_assert(m == NULL, "Should not match.");

    return NULL;
}

char *test_escaped()
{
    // char classes, which map to C functions:
    // \a : isalpha(c)
    // \c : iscntrl(c)
    // \d : isdigit(c)
    // \l : islower(c)
    // \p : ispunct(c)
    // \s : isspace(c)
    // \u : isupper(c)
    // \w : isalnum(c)
    // \x : isxdigit(c)
    // \z : (c == 0)  // NUL terminator

    m = pattern_match("a \t 9 l . \n U w A \0", 19, "\\a \\c \\d \\l \\p \\s \\u \\w \\x \\z");

    mu_assert(m != NULL, "Should match.");

    // escaped, non-class characters map to themselves

    m = pattern_match("ZED", strlen("ZED"), "Z\\ED");

    mu_assert(m != NULL, "Should match.");

    // escaped special characters are depowered

    m = pattern_match("*Z*E+D.", strlen("*Z*E+D."), "\\*Z\\*E\\+D\\.");

    mu_assert(m != NULL, "Should match.");

    m = pattern_match("ZED!", strlen("ZED!"), "ZED\\.");

    mu_assert(m == NULL, "Should not match.");

    return NULL;
}


char * all_tests() {
    mu_suite_start();

    mu_run_test(test_simple_matches);
    mu_run_test(test_simple_repetitions);
    mu_run_test(test_anchors);
    mu_run_test(test_blocks);
    mu_run_test(test_frontier);
    mu_run_test(test_balance);
    mu_run_test(test_set);
    mu_run_test(test_optional);
    mu_run_test(test_escaped);

    return NULL;
}

RUN_TESTS(all_tests);


