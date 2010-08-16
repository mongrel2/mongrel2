#include "minunit.h"
#include <cache.h>
#include <assert.h>

FILE *LOG_FILE = NULL;

static long last_evicted;
void test_evict(void *data) {
    last_evicted = (long) data;
}

int test_lookup(void *data, void *key) {
    return data == key;
}

char *test_cache_evict()
{
    last_evicted = -1;

    Cache *cache = Cache_create(MIN_CACHE_SIZE, test_lookup, test_evict);
    mu_assert(cache != NULL, "Failed to create cache");
    
    long i;
    for(i = 1; i <= MIN_CACHE_SIZE; i++) {
        Cache_add(cache, (void *) i);
        mu_assert(last_evicted == -1, "Evicted something too early");
    }

    for(i = 1; i <= MIN_CACHE_SIZE; i++) {
        Cache_add(cache, (void *) i + MIN_CACHE_SIZE);
        mu_assert(last_evicted == i, "Evicted something out of order");
    }
    
    Cache_destroy(cache);
    return NULL;
}

char *test_cache_manual_evict()
{
    last_evicted = -1;

    Cache *cache = Cache_create(MIN_CACHE_SIZE, test_lookup, test_evict);
    mu_assert(cache != NULL, "Failed to create cache");
    
    long i;
    for(i = 1; i <= MIN_CACHE_SIZE; i++) {
        Cache_add(cache, (void *) i);
        mu_assert(last_evicted == -1, "Evicted something too early");
    }


    for(i = 1; i <= MIN_CACHE_SIZE; i++) {
        Cache_evict_object(cache, (void *) i);
        mu_assert(last_evicted == i, "Evicted something out of order");
    }

    last_evicted = -1;
    for(i = 1; i <= MIN_CACHE_SIZE; i++) {
        Cache_add(cache, (void *) i);
        mu_assert(last_evicted == -1, "The cache wasn't empty'");
    }

    Cache_destroy(cache);
    return NULL;
}

char *test_cache_lookup()
{
    Cache *cache = Cache_create(MIN_CACHE_SIZE, test_lookup, test_evict);
    mu_assert(cache != NULL, "Failed to create cache");
    long item;

    long i;
    for(i = 1; i <= MIN_CACHE_SIZE; i++) {
        item = (long) Cache_lookup(cache, (void *) i);
        mu_assert(item == 0, "Found something that wasn't there");
    }

    for(i = 1; i <= MIN_CACHE_SIZE; i++) Cache_add(cache, (void *) i);
    
    for(i = 1; i <= MIN_CACHE_SIZE; i++) {
        item = (long) Cache_lookup(cache, (void *) i);
        mu_assert(item == i, "Did not find something that should be there");
    }

    for(i = 1; i <= MIN_CACHE_SIZE; i++) Cache_add(cache, (void *) (i * 2));

    for(i = 1; i <= MIN_CACHE_SIZE; i++) {
        long f = i * 2;
        item = (long) Cache_lookup(cache, (void *) f);
        mu_assert(item == f, "Did not find something that should be there");
        long nf = f - 1;
        item = (long) Cache_lookup(cache, (void *) nf);
        mu_assert(item == 0, "Found something that wasn't there");
    }

    Cache_destroy(cache);

    return NULL;
}

char *all_tests() {
    mu_suite_start();
    
    mu_run_test(test_cache_evict);
    mu_run_test(test_cache_manual_evict);
    mu_run_test(test_cache_lookup);

    return NULL;
}

RUN_TESTS(all_tests);
