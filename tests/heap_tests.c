#include "minunit.h"
#include <adt/heap.h>

FILE *LOG_FILE = NULL;

char *test_Heap_create_destroy()
{
    Heap *h = Heap_create(100);
    mu_assert(h != NULL, "Failed to make Heap.");
    mu_assert(h->keys != NULL, "Failed to make internals.");
    mu_assert(Heap_capacity(h) == 100, "Wrong dimensions.");
    mu_assert(Heap_length(h) == 0, "Not initialized.");

    Heap_destroy(h);

    // this is legal too
    Heap_destroy(NULL);

    return NULL;
}


char *test_Heap_add()
{
    Heap *h = Heap_create(100);
    int i = 0;

    Heap_add(h, 1);
    Heap_add(h, 1);
    Heap_add(h, 1);
    Heap_add(h, 1);

    mu_assert(Heap_length(h) == 4, "Wrong length, should be 4.");

    for(i = 0; i < Heap_length(h); i++) {
        debug("H: %d=%d", i, Heap_at(h, i));
        mu_assert(Heap_at(h, i) == 1, "Wrong number, should be 1.");
    }

    Heap_add(h, 5);
    Heap_add(h, 10);
    Heap_add(h, 3);
    Heap_add(h, 6);
    Heap_add(h, 13);
    
    mu_assert(Heap_length(h) == 9, "Wrong length, should be 4.");

    debug("\nHEAP:");
    for(i = 0; i < Heap_length(h); i++) {
        debug("H: %d=%d", i, Heap_at(h, i));
    }

    Heap_destroy(h);
    return NULL;
}

char *test_Heap_delete()
{
    Heap *h = Heap_create(100);
    int i = 0;

    for(i = 0; i < 10; i++) {
        Heap_add(h, i);
        mu_assert(Heap_length(h) == i+1, "Wrong length.");
    }

    // delete from end
    Heap_delete(h, Heap_length(h) - 1);
    mu_assert(Heap_length(h) == 9, "Wrong length after delete.");
    mu_assert(Heap_at(h, 9) == 9, "Last one should be 9.");

    // delete from front
    for(i = 0; i < 8; i++) {
        int old = Heap_at(h, 0 % 2);
        Heap_delete(h, 0 % 2);
        mu_assert(old != Heap_at(h, i), "Should be different after delete.");
    }

    mu_assert(Heap_length(h) == 1, "Should be one left.");
    Heap_delete(h, 0);

    mu_assert(Heap_is_empty(h), "Should be empty.");

    Heap_destroy(h);
    return NULL;
}

char *test_Heap_search()
{
    Heap *h = Heap_create(100);
    int i = 0;

    for(i = 0; i < 10; i++) {
        Heap_add(h, i);
        mu_assert(Heap_length(h) == i+1, "Wrong length.");
    }

    mu_assert(Heap_search(h, 3) == 3, "Didn't find it 3");
    mu_assert(Heap_search(h, 0) == 0, "Didn't find it 0");
    mu_assert(Heap_search(h, 9) == 9, "Didn't find it 0");
    mu_assert(Heap_search(h, 100) == -1, "Should not find 100.");

    Heap_destroy(h);

    return NULL;
}


char * all_tests() {
    mu_suite_start();

    mu_run_test(test_Heap_create_destroy);
    mu_run_test(test_Heap_add);
    mu_run_test(test_Heap_delete);
    mu_run_test(test_Heap_search);

    return NULL;
}

RUN_TESTS(all_tests);

