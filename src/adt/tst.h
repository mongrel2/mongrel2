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
