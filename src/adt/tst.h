#ifndef tst_h
#define tst_h

#include <stdlib.h>
#include <adt/list.h>

typedef struct tst_t { 
    char splitchar; 
    struct tst_t *low;
    struct tst_t *equal;
    struct tst_t *high; 
    void *value;
} tst_t; 


typedef void (*tst_traverse_cb)(void *value, void *data);
typedef int (*tst_collect_test_cb)(void *value, const char *path, size_t len);


// won't work unless you reverse before insert, useful though
// for looking up things from last to first char, as in hostnames
void *tst_search_suffix(tst_t *root, const char *s, size_t len);

void *tst_search(tst_t *root, const char *s, size_t len);

void *tst_search_prefix(tst_t *root, const char *s, size_t len);

tst_t *tst_insert(tst_t *p, const char *s, size_t len, void *value);

// TODO: should pass in the key as well
void tst_traverse(tst_t *p, tst_traverse_cb cb, void *data);

list_t *tst_collect(tst_t *root, const char *s, size_t len, tst_collect_test_cb tester);

void tst_destroy(tst_t *root);

#endif
