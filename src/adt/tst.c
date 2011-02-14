/**
 *
 * Copyright (c) 2010, Zed A. Shaw and Mongrel2 Project Contributors.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 * 
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 * 
 *     * Neither the name of the Mongrel2 Project, Zed A. Shaw, nor the names
 *       of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written
 *       permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

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

enum {
    MAX_TRAVERSE_SIZE = 128
};

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

    while(i >= 0 && p) {
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


tst_t **tst_resize_queue(tst_t **queue, int q_start, int q_size, int q_max, int new_size)
{
    int i = 0;
    tst_t ** new_queue = calloc(sizeof(tst_t *),  new_size);
    check(new_queue, "Failed to reallocate queue for traverse");

    for(i = 0; i < q_size; i++) {
        new_queue[i] = queue[(i + q_start) % q_max];
    }

    free(queue);
    return new_queue;

error:
    free(queue);
    return NULL;
}



void tst_traverse(tst_t *p, tst_traverse_cb cb, void *data)
{
    if (!p) return;

    int q_start = 0;
    int q_size = 0;
    int q_max = MAX_TRAVERSE_SIZE;
    tst_t **queue = calloc(sizeof(tst_t *), MAX_TRAVERSE_SIZE);

    check(queue, "Failed to malloc queue for traverse");

    queue[q_start] = p;
    q_size++;

    while(q_size > 0) {
        tst_t *cur = queue[q_start];
        q_start = (q_start + 1) % q_max;
        q_size--;

        // Resize if we must
        if(q_size + 3 > q_max) {
            queue = tst_resize_queue(queue, q_start, q_size, q_max, q_max * 2);
            q_start = 0;
            q_max = q_max * 2;
        }

        if(cur->value) cb(cur->value, data);

        if(cur->low) queue[(q_start + (q_size++)) % q_max] = cur->low;
        if(cur->equal) queue[(q_start + (q_size++)) % q_max] = cur->equal;
        if(cur->high) queue[(q_start + (q_size++)) % q_max] = cur->high;
    }

    free(queue);
    return;

error:
    if(queue) free(queue);
}


void tst_destroy(tst_t *root)
{
    if(root) {
        h_free(root);
    }
}
