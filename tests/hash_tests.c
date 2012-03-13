#include "minunit.h"
#include <register.h>
#include <adt/hash.h>

#include <stdio.h>
#include <ctype.h>
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

static char *dupstring(char *str)
{
    int sz = strlen(str) + 1;
    char *new = malloc(sz);
    if (new)
        memcpy(new, str, sz);
    return new;
}

static hnode_t *new_node(void *c)
{
    (void)c;

    static hnode_t few[5];
    static int count;

    if (count < 5)
        return few + count++;

    return NULL;
}

static void del_node(hnode_t *n, void *c)
{
    (void)n;
    (void)c;
}

char *test_hash_operations()
{
    input_t in;
    hash_t *h = hash_create(HASHCOUNT_T_MAX, 0, 0);
    hnode_t *hn;
    hscan_t hs;
    char *tok1, *tok2, *val;
    const char *key;
    int prompt = 0;
    FILE *data = fopen("tests/adt_data.txt", "r");
    mu_assert(data != NULL, "Couldn't load test data.");

    char *help =
        "a <key> <val>          add value to hash table\n"
        "d <key>                delete value from hash table\n"
        "l <key>                lookup value in hash table\n"
        "n                      show size of hash table\n"
        "c                      show number of entries\n"
        "t                      dump whole hash table\n"
        "+                      increase hash table (private func)\n"
        "-                      decrease hash table (private func)\n"
        "b                      print hash_t_bit value\n"
        "p                      turn prompt on\n"
        "s                      switch to non-functioning allocator\n"
        "q                      quit";

    if (!h)
        puts("hash_create failed");

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
            case 'b':
                printf("%d\n", hash_val_t_bit);
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

                if (!hash_alloc_insert(h, key, val)) {
                    puts("hash_alloc_insert failed");
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
                hn = hash_lookup(h, tok1);
                if (!hn) {
                    break;
                }
                val = hnode_get(hn);
                key = hnode_getkey(hn);
                hash_scan_delfree(h, hn);
                free((void *) key);
                free(val);
                break;
            case 'l':
                if (tokenize(in+1, &tok1, (char **) 0) != 1) {
                    puts("what?");
                    break;
                }
                hn = hash_lookup(h, tok1);
                if (!hn) {
                    break;
                }
                val = hnode_get(hn);
                puts(val);
                break;
            case 'n':
                printf("%lu\n", (unsigned long) hash_size(h));
                break;
            case 'c':
                printf("%lu\n", (unsigned long) hash_count(h));
                break;
            case 't':
                hash_scan_begin(&hs, h);
                while ((hn = hash_scan_next(&hs))) {
                    mu_assert(hnode_getkey(hn) != NULL, "invalid key");
                    mu_assert(hnode_get(hn) != NULL, "invalid value");
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
                hash_set_allocator(h, new_node, del_node, NULL);
                break;
            default:
                putchar('?');
                putchar('\n');
                break;
        }
    }

    mu_assert(hash_verify(h), "Hash became invalid.");

    return NULL;
}

char * all_tests() {
    mu_suite_start();

    mu_run_test(test_hash_operations);

    return NULL;
}

RUN_TESTS(all_tests);
