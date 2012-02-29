#include "minunit.h"
#include <adt/darray.h>

char *test_darray_operations()
{
    darray_t *array = darray_create(sizeof(int), 100);
    mu_assert(array != NULL, "darray_create failed.");
    mu_assert(array->contents != NULL, "contents are wrong in darray");
    mu_assert(array->end == 0, "end isn't at the right spot");
    mu_assert(array->element_size == sizeof(int), "element size is wrong.");
    mu_assert(array->max == 100, "wrong max length on initial size");

    int *val1 = darray_new(array);
    mu_assert(val1 != NULL, "failed to make a new element");

    int *val2 = darray_new(array);
    mu_assert(val2 != NULL, "failed to make a new element");

    darray_set(array, 0, val1);
    darray_set(array, 1, val2);

    mu_assert(darray_get(array, 0) == val1, "Wrong first value.");
    mu_assert(darray_get(array, 1) == val2, "Wrong second value.");

    int *val_check = darray_remove(array, 0);
    mu_assert(val_check != NULL, "Should not get NULL.");
    mu_assert(*val_check == *val1, "Should get the first value.");
    mu_assert(darray_get(array, 0) == NULL, "Should be gone.");
    darray_free(val_check);

    val_check = darray_remove(array, 1);
    mu_assert(val_check != NULL, "Should not get NULL.");
    mu_assert(*val_check == *val2, "Should get the first value.");
    mu_assert(darray_get(array, 1) == NULL, "Should be gone.");
    darray_free(val_check);

    signed int old_max = array->max;
    darray_expand(array);
    mu_assert(array->max == old_max + array->expand_rate, "Wrong size after expand.");

    darray_contract(array);
    mu_assert(array->max == array->expand_rate + 1, "Should stay at the expand_rate at least.");

    darray_contract(array);
    mu_assert(array->max == array->expand_rate + 1, "Should stay at the expand_rate at least.");

    int i = 0;
    for(i = 0; i < 1000; i++) {
        int *val = darray_new(array);
        darray_attach(array, val); 
        *val = i * 333;
        darray_push(array, val);
    }

    mu_assert(array->max == 1201, "Wrong max size.");

    for(i = 999; i > 0; i--) {
        int *val = darray_pop(array);
        mu_assert(val != NULL, "Shouldn't get a NULL.");
        mu_assert(*val == i * 333, "Wrong value.");
        darray_free(val);
    }

    darray_destroy(array);
 
    return NULL;
}


char * all_tests() {
    mu_suite_start();

    mu_run_test(test_darray_operations);

    return NULL;
}

RUN_TESTS(all_tests);

