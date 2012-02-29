#include "minunit.h"
#include <register.h>
#include <adt/dict.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>

typedef char input_t[256];

static int tokenize(char *string, ...)
{
    char **tokptr; 
    va_list arglist;
    int tokcount = 0;

    va_start(arglist, string);
    tokptr = va_arg(arglist, char **);
    while (tokptr) {
        while (*string && isspace((unsigned char) *string))
            string++;
        if (!*string)
            break;
        *tokptr = string;
        while (*string && !isspace((unsigned char) *string))
            string++;
        tokptr = va_arg(arglist, char **);
        tokcount++;
        if (!*string)
            break;
        *string++ = 0;
    }
    va_end(arglist);

    return tokcount;
}

static int comparef(const void *key1, const void *key2)
{
    return strcmp(key1, key2);
}

static char *dupstring(char *str)
{
    int sz = strlen(str) + 1;
    char *new = malloc(sz);
    if (new)
        memcpy(new, str, sz);
    return new;
}

static dnode_t *new_node(void *c)
{
    (void)c;

    static dnode_t few[5];
    static int count;

    if (count < 5)
        return few + count++;

    return NULL;
}

static void del_node(dnode_t *n, void *c)
{
    (void)n;
    (void)c;
}

static int prompt = 0;

static void construct(dict_t *d)
{
    input_t in;
    int done = 0;
    dict_load_t dl;
    dnode_t *dn;
    char *tok1, *tok2, *val;
    const char *key;
    char *help = 
        "p                      turn prompt on\n"
        "q                      finish construction\n"
        "a <key> <val>          add new entry\n";

    if (!dict_isempty(d))
        puts("warning: dictionary not empty!");

    dict_load_begin(&dl, d);

    while (!done) {
        if (prompt)
            putchar('>');
        fflush(stdout);

        if (!fgets(in, sizeof(input_t), stdin))
            break;

        switch (in[0]) {
            case '?':
                puts(help);
                break;
            case 'p':
                prompt = 1;
                break;
            case 'q':
                done = 1;
                break;
            case 'a':
                if (tokenize(in+1, &tok1, &tok2, (char **) 0) != 2) {
                    puts("what?");
                    break;
                }
                key = dupstring(tok1);
                val = dupstring(tok2);
                dn = dnode_create(val);

                if (!key || !val || !dn) {
                    puts("out of memory");
                    free((void *) key);
                    free(val);
                    if (dn)
                        dnode_destroy(dn);
                }

                dict_load_next(&dl, dn, key);
                break;
            default:
                putchar('?');
                putchar('\n');
                break;
        }
    }

    dict_load_end(&dl);
}

char *test_dict_operations()
{
    input_t in;
    dict_t darray[10];
    dict_t *d = &darray[0];
    dnode_t *dn;
    unsigned int i;
    char *tok1, *tok2, *val;
    const char *key;
    FILE *data = fopen("tests/adt_data.txt", "r");
    mu_assert(data != NULL, "Couldn't load test data.");

    char *help =
        "a <key> <val>          add value to dictionary\n"
        "d <key>                delete value from dictionary\n"
        "l <key>                lookup value in dictionary\n"
        "( <key>                lookup lower bound\n"
        ") <key>                lookup upper bound\n"
        "# <num>                switch to alternate dictionary (0-9)\n"
        "j <num> <num>          merge two dictionaries\n"
        "f                      free the whole dictionary\n"
        "k                      allow duplicate keys\n"
        "c                      show number of entries\n"
        "t                      dump whole dictionary in sort order\n"
        "m                      make dictionary out of sorted items\n"
        "p                      turn prompt on\n"
        "s                      switch to non-functioning allocator\n"
        "q                      quit";

    for (i = 0; i < sizeof darray / sizeof *darray; i++)
        dict_init(&darray[i], DICTCOUNT_T_MAX, comparef);

    for (;;) {
        if (prompt)
            putchar('>');
        fflush(stdout);

        if (!fgets(in, sizeof(input_t), data))
            break;

        switch(in[0]) {
            case '?':
                puts(help);
                break;
            case 'a':
                if (tokenize(in+1, &tok1, &tok2, (char **) 0) != 2) {
                    puts("what?");
                    break;
                }
                key = dupstring(tok1);
                val = dupstring(tok2);

                if (!key || !val) {
                    puts("out of memory");
                    free((void *) key);
                    free(val);
                }

                if (!dict_alloc_insert(d, key, val)) {
                    puts("dict_alloc_insert failed");
                    free((void *) key);
                    free(val);
                    break;
                }
                break;
            case 'd':
                if (tokenize(in+1, &tok1, (char **) 0) != 1) {
                    puts("what?");
                    break;
                }
                dn = dict_lookup(d, tok1);
                if (!dn) {
                    break;
                }
                val = dnode_get(dn);
                key = dnode_getkey(dn);
                dict_delete_free(d, dn);

                free(val);
                free((void *) key);
                break;
            case 'f':
                dict_free(d);
                break;
            case 'l':
            case '(':
            case ')':
                if (tokenize(in+1, &tok1, (char **) 0) != 1) {
                    puts("what?");
                    break;
                }
                dn = 0;
                switch (in[0]) {
                    case 'l':
                        dn = dict_lookup(d, tok1);
                        break;
                    case '(':
                        dn = dict_lower_bound(d, tok1);
                        break;
                    case ')':
                        dn = dict_upper_bound(d, tok1);
                        break;
                }
                if (!dn) {
                    break;
                }
                val = dnode_get(dn);
                puts(val);
                break;
            case 'm':
                construct(d);
                break;
            case 'k':
                dict_allow_dupes(d);
                break;
            case 'c':
                printf("%lu\n", (unsigned long) dict_count(d));
                break;
            case 't':
                for (dn = dict_first(d); dn; dn = dict_next(d, dn)) {
                    mu_assert(dnode_getkey(dn) != NULL, "invalid key");
                    mu_assert(dnode_get(dn) != NULL, "invalid value");
                }
                break;
            case 'q':
                return NULL;
                break;
            case '\0':
                break;
            case 'p':
                prompt = 1;
                break;
            case 's':
                dict_set_allocator(d, new_node, del_node, NULL);
                break;
            case '#':
                if (tokenize(in+1, &tok1, (char **) 0) != 1) {
                    puts("what?");
                    break;
                } else {
                    int dictnum = atoi(tok1);
                    if (dictnum < 0 || dictnum > 9) {
                        puts("invalid number");
                        break;
                    }
                    d = &darray[dictnum];
                }
                break;
            case 'j':
                if (tokenize(in+1, &tok1, &tok2, (char **) 0) != 2) {
                    puts("what?");
                    break;
                } else {
                    int dict1 = atoi(tok1), dict2 = atoi(tok2);
                    if (dict1 < 0 || dict1 > 9 || dict2 < 0 || dict2 > 9) {
                        puts("invalid number");
                        break;
                    }
                    dict_merge(&darray[dict1], &darray[dict2]);
                }
                break;
            default:
                putchar('?');
                putchar('\n');
                break;
        }
    }

    mu_assert(dict_verify(d), "Dict became invalid.");

    return NULL;
}

char * all_tests() {
    mu_suite_start();

    mu_run_test(test_dict_operations);

    return NULL;
}

RUN_TESTS(all_tests);
