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

#include "adt/darray.h"
#include "mem/halloc.h"
#include <assert.h>


darray_t *darray_create(size_t element_size, size_t initial_max)
{
    darray_t *array = h_malloc(sizeof(darray_t));
    check_mem(array);
    array->max = initial_max;
    check(array->max > 0, "You must set an initial_max > 0.");

    array->contents = h_calloc(sizeof(void *), initial_max);
    check_mem(array->contents);
    hattach(array->contents, array);

    array->end = 0;
    array->element_size = element_size;
    array->expand_rate = DEFAULT_EXPAND_RATE;

    return array;

error:
    if(array) h_free(array);
    return NULL;
}

void darray_clear(darray_t *array)
{
    int i = 0;
    if(array->element_size > 0) {
        for(i = 0; i < array->max; i++) {
            if(array->contents[i] != NULL) {
                free(array->contents[i]);
            }
        }
    }
}

static inline int darray_resize(darray_t *array, size_t newsize)
{
    array->max = newsize;
    check(array->max > 0, "The newsize must be > 0.");
    array->contents = h_realloc(array->contents, array->max * sizeof(void *));
    check_mem(array->contents);
    return 0;
error:
    return -1;
}

int darray_expand(darray_t *array)
{
    size_t old_max = array->max;
    check(darray_resize(array, array->max + array->expand_rate) == 0,
            "Failed to expand array to new size: %d",
            array->max + (int)array->expand_rate);

    memset(array->contents + old_max, 0, array->expand_rate * sizeof(void *));
    return 0;

error:
    return -1;
}

int darray_contract(darray_t *array)
{
    int new_size = array->end < (int)array->expand_rate ? (int)array->expand_rate : array->end;

    return darray_resize(array, new_size + 1);
}


void darray_destroy(darray_t *array)
{
    h_free(array);
}

void darray_clear_destroy(darray_t *array)
{
    darray_clear(array);
    darray_destroy(array);
}

int darray_push(darray_t *array, void *el)
{
    array->contents[array->end] = el;
    array->end++;

    if(darray_end(array) >= darray_max(array)) {
        return darray_expand(array);
    } else {
        return 0;
    }
}

void *darray_pop(darray_t *array)
{
    check(array->end - 1 >= 0, "Attempt to pop from empty array.");

    void *el = darray_remove(array, array->end - 1);
    array->end--;

    if(darray_end(array) > (int)array->expand_rate && darray_end(array) % array->expand_rate) {
        darray_contract(array);
    }

    return el;
error:
    return NULL;
}

int darray_insert(darray_t *array, int i, void *el)
{
    array->end++;

    if(darray_end(array) >= darray_max(array)) {
        if(darray_expand(array) != 0) {
            return -1;
        }
    }

    int n;
    for(n = array->end - 1; n > i; n--) {
        array->contents[n] = array->contents[n - 1];
    }

    array->contents[i] = el;

    return 0;
}

void darray_move_to_end(darray_t *array, int i)
{
    void *el = array->contents[i];

    int n;
    for(n = i + 1; n < array->end; n++) {
        array->contents[n - 1] = array->contents[n];
    }

    array->contents[array->end - 1] = el;
}

void darray_remove_and_resize(darray_t *array, int start, int count)
{
    int n;
    if(array->element_size > 0) {
        for(n = start; n < (start + count); n++) {
            if(array->contents[n] != NULL) {
                h_free(array->contents[n]);
            }
        }
    }

    for(n = start + count; n < array->end; n++) {
        int i = n - (start + count);
        array->contents[start + i] = array->contents[n];
        array->contents[n] = NULL;
    }

    array->end = array->end - count;

    if(darray_end(array) > (int)array->expand_rate && darray_end(array) % array->expand_rate) {
        darray_contract(array);
    }
}
