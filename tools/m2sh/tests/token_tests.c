#include "minunit.h"
#include "token.h"
#include <stdio.h>
#include <bstring.h>

FILE *LOG_FILE = NULL;

char *test_tokenize() 
{
    FILE *script = fopen("tests/sample.conf", "r");
    mu_assert(script != NULL, "Failed to open file");

    bstring buffer = bread((bNread)fread, script);
    mu_assert(buffer != NULL, "Failed to read file");

    fclose(script);

    list_t *tokens = Lexer_tokenize(buffer);
    mu_assert(tokens != NULL, "Lexer failed.");

    debug("GOT %d tokens", (int)list_count(tokens));

    bdestroy(buffer);

    return NULL;
}



char * all_tests() {
    mu_suite_start();

    mu_run_test(test_tokenize);

    return NULL;
}

RUN_TESTS(all_tests);

