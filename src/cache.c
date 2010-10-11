#include "cache.h"

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "dbg.h"

Cache *Cache_create(int size, cache_lookup_cb lookup, cache_evict_cb evict)
{
    Cache *cache = NULL;
    size_t extra_size = 0;
    int i = 0;

    check(lookup, "lookup passed to cache_create is NULL");

    if(size > MIN_CACHE_SIZE) {
        extra_size = (size - MIN_CACHE_SIZE) * sizeof(struct cache_entry);
    }

    cache = calloc(sizeof(Cache) + extra_size, 1);
    check_mem(cache);

    cache->size = size;
    cache->lookup = lookup;
    cache->evict = evict;
    
    for(i = 0; i < size; i++) {
        cache->arr[i].tag = INT_MAX;
    }

    return cache;

error:
    return NULL;
}

void Cache_destroy(Cache *cache)
{
    int i = 0;
    check(cache, "NULL cache argument to Cache_destroy");

    if(cache->evict) {
        for(i = 0; i < cache->size; i++) {
            if(cache->arr[i].data) {
                cache->evict(cache->arr[i].data);
            }
        }
    }
    
    free(cache);

error: // fallthrough
    return;
}

void *Cache_lookup(Cache *cache, void *key)
{
    check(cache, "NULL cache argument to Cache_lookup");

    void *rdata = NULL;
    int i = 0;

    for(i = 0; i < cache->size; i++) {
        if(cache->arr[i].tag > 0) {
            cache->arr[i].tag--;
        }

        rdata = cache->arr[i].data;

        if(rdata && cache->lookup(rdata, key)) {
            cache->arr[i].tag = INT_MAX;
            break;
        }
        rdata = NULL;
    }

    for(i = i + 1; i < cache->size; i++) {
        if(cache->arr[i].tag > 0) {
            cache->arr[i].tag--;
        }
    }

    return rdata;

error:
    return NULL;
}

void Cache_add(Cache *cache, void *data)
{
    check(cache, "NULL cache argument to Cache_add");
    check(data, "Cannot add NULL as data to cache");

    int i = 0;
    int min_idx = 0;
    int min_tag = cache->arr[min_idx].tag;

    if(cache->arr[0].tag > 0) {
        cache->arr[0].tag--;
    }

    for(i = 1; i < cache->size; i++) {
        if(cache->arr[i].tag < min_tag) {
            min_tag = cache->arr[i].tag;
            min_idx = i;
        }

        if(cache->arr[i].tag > 0) {
            cache->arr[i].tag--;
        }
    }
    
    if(cache->arr[min_idx].data && cache->evict) {
        cache->evict(cache->arr[min_idx].data);
    }

    cache->arr[min_idx].data = data;
    cache->arr[min_idx].tag = INT_MAX;
   
error: // fallthrough
    return;
}

void Cache_evict_object(Cache *cache, void *obj)
{
    int i = 0;

    check(cache, "NULL cache argument to Cache_evict_object");
    check(obj, "NULL obj argument to Cache_evict_object");

    for(i = 0; i < cache->size; i++) {
        if(cache->arr[i].data == obj) {
            if(cache->evict) {
                cache->evict(cache->arr[i].data);
            }

            cache->arr[i].data = NULL;
            cache->arr[i].tag = 0;
        }
    }

error:
    return;
}
