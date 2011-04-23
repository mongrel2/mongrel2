#undef NDEBUG

#include "adt/darray.h"
#include "mem/halloc.h"
#include "dbg.h"
#include <assert.h>


darray_t *darray_create(size_t element_size, size_t initial_max)
{
    darray_t *array = h_malloc(sizeof(darray_t));
    check_mem(array);

    array->contents = h_calloc(sizeof(void *), initial_max);
    check_mem(array->contents);
    hattach(array->contents, array);

    array->max = initial_max;
    array->end = 0;
    array->element_size = element_size;
    array->expand_rate = DEFAULT_EXPAND_RATE;

    return array;

error:
    if(array) h_free(array);
    return NULL;
}

void *darray_new(darray_t *array)
{
    return calloc(1, array->element_size);
}

void darray_clear(darray_t *array)
{
    int i = 0;

    for(i = 0; i < array->max; i++) {
        if(array->contents[i] != NULL) {
            free(array->contents[i]);
        }
    }
}

static inline int darray_resize(darray_t *array, size_t newsize)
{
    array->max = newsize;
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
            (int)array->max + (int)array->expand_rate);

    memset(array->contents + old_max, 0, array->expand_rate + 1);
    return 0;

error:
    return -1;
}

int darray_contract(darray_t *array)
{
    int new_size = array->end < array->expand_rate ? array->expand_rate : array->end;

    return darray_resize(array, new_size + 1);
}


void darray_destroy(darray_t *array)
{
    darray_clear(array);
    h_free(array);
}

void darray_set(darray_t *array, int i, void *el)
{
    check(i < array->max, "darray attempt to set past max");
    array->contents[i] = el;
error:
    return;
}

void *darray_get(darray_t *array, int i)
{
    assert(i < array->max && "darray attempt to get past max");
    return array->contents[i];
}

void *darray_remove(darray_t *array, int i)
{
    void *el = array->contents[i];

    array->contents[i] = NULL;

    return el;
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
    check(array->end - 1 > 0, "Attempt to pop from empty array.");

    void *el = darray_remove(array, array->end - 1);
    array->end--;

    if(darray_end(array) > array->expand_rate && darray_end(array) % array->expand_rate) {
        darray_contract(array);
    }

    return el;
error:
    return NULL;
}


