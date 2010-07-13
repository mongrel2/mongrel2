#include <stdlib.h>
#include "tst.h"
#include <stdio.h>
#include <assert.h>
#include <dbg.h>
#include <mem/halloc.h>

typedef struct tst_collect_t {
    list_t *values;
    tst_collect_test_cb tester;
    const char *key;
    size_t len;
} tst_collect_t;


static void tst_collect_build(void *value, tst_collect_t *results)
{
    if(!results->tester || results->tester(value, results->key, results->len)) {
        lnode_t *node = lnode_create(value);
        list_append(results->values, node);
    }
}

// TODO: give this a reversed parameter for reversed collects, similar to reversed search
list_t *tst_collect(tst_t *root, const char *s, size_t len, tst_collect_test_cb tester)
{
    tst_collect_t results = {.values = NULL, .tester = tester, .key = s, .len = len};
    tst_t *p = root;
    tst_t *last = p;
    int i = 0;
    results.values = list_create(LISTCOUNT_T_MAX);

    // first we get to where we match the prefix
    while(i < len && p) {
        if (s[i] < p->splitchar) {
            p = p->low; 
        } else if (s[i] == p->splitchar) {
            i++;
            if(i < len) {
                if(p->value) last = p;
                p = p->equal; 
            }
        } else {
            p = p->high; 
        }
    }

    if((last && results.tester) || p) {
        // we found node matching this prefix, so traverse and collect
        tst_traverse(p == NULL ? last : p, (tst_traverse_cb)tst_collect_build, &results);
    }

    return results.values;
}

void *tst_search_suffix(tst_t *root, const char *s, size_t len)
{
    if(len == 0) return NULL;

    tst_t *p = root;
    tst_t *last = NULL;
    int i = len-1;

    while(i > 0 && p) {
        if (s[i] < p->splitchar) {
            p = p->low; 
        } else if (s[i] == p->splitchar) {
            i--;
            if(i >= 0) {
                if(p->value) last = p;
                p = p->equal;
            }
        } else {
            p = p->high; 
        }
    }

    if(p) {
        return p->value;
    } else if(last) {
        return last->value;
    } else {
        return NULL;
    }
}

void *tst_search_prefix(tst_t *root, const char *s, size_t len)
{
    if(len == 0) return NULL;

    tst_t *p = root;
    tst_t *last = NULL;
    int i = 0;

    while(i < len && p) {
        if (s[i] < p->splitchar) {
            p = p->low; 
        } else if (s[i] == p->splitchar) {
            i++;
            if(i < len) {
                if(p->value) last = p;
                p = p->equal;
            }
        } else {
            p = p->high; 
        }
    }

    if(p) {
        return p->value;
    } else if(last) {
        return last->value;
    } else {
        return NULL;
    }
}

void *tst_search(tst_t *root, const char *s, size_t len)
{
    tst_t *p = root;
    int i = 0;

    while(i < len && p) {

        if (s[i] < p->splitchar) {
            p = p->low; 
        } else if (s[i] == p->splitchar) {
            i++;
            if(i < len) p = p->equal; 
        } else {
            p = p->high; 
        }
    }

    if(p) {
        return p->value;
    } else {
        return NULL; 
    }
}


tst_t *tst_insert_base(tst_t *root, tst_t *p, const char *s, size_t len, void *value)
{
    if (p == NULL) { 
        p = (tst_t *) h_calloc(sizeof(tst_t), 1);

        if(root == NULL) {
            root = p;
        } else {
            hattach(p, root);
        }

        p->splitchar = *s; 
    }

    if (*s < p->splitchar) {
        p->low = tst_insert_base(root, p->low, s, len, value); 
    } else if (*s == p->splitchar) { 
        if (len > 1) {
            // not done yet, keep going but one less
            p->equal = tst_insert_base(root, p->equal, s+1, len - 1, value);
        } else {
            assert(p->value == NULL && "Duplicate insert into tst.");
            p->value = value;
        }
    } else {
        p->high = tst_insert_base(root, p->high, s, len, value);
    }

    return p; 
}

tst_t *tst_insert(tst_t *p, const char *s, size_t len, void *value)
{
    return tst_insert_base(p, p, s, len, value);
}

void tst_traverse(tst_t *p, tst_traverse_cb cb, void *data)
{
    if (!p) return;

    int qStart = 0, qSize = 0, maxSize = 128;
    tst_t **myQueue = malloc(sizeof(tst_t *) * maxSize);
    check(myQueue, "Failed to malloc queue for traverse");

    myQueue[qStart] = p;
    qSize++;

    while(qSize > 0) {
        tst_t *cur = myQueue[qStart];
        qStart = (qStart + 1) % maxSize;
        qSize--;

        // Resize if we must
        if(qSize + 3 > maxSize) {
            int i, newSize = maxSize * 2;
            tst_t **newQueue = malloc(sizeof(tst_t *) * newSize);
            check(newQueue, "Failed to reallocate queue for traverse");
            for(i = 0; i < qSize; i++)
                newQueue[i] = myQueue[(i + qStart) % maxSize];
            free(myQueue);
            qStart = 0;
            myQueue = newQueue;
            maxSize = newSize;
        }
        if(cur->value) cb(cur->value, data);

        if(cur->low) myQueue[(qStart + (qSize++)) % maxSize] = cur->low;
        if(cur->equal) myQueue[(qStart + (qSize++)) % maxSize] = cur->equal;
        if(cur->high) myQueue[(qStart + (qSize++)) % maxSize] = cur->high;
    }

    free(myQueue);
    return;

error:
    if(myQueue) free(myQueue);
}


void tst_destroy(tst_t *root)
{
    if(root) {
        h_free(root);
    }
}
