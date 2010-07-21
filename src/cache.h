#ifndef _CACHE_H
#define _CACHE_H

#define MIN_CACHE_SIZE 16


typedef int (*cache_lookup_cb)(void *data, void *key);
typedef void (*cache_evict_cb)(void *data);

struct cache_entry {
    int tag;
    void *data;
};

typedef struct Cache {
    cache_lookup_cb lookup;
    cache_evict_cb evict;
    int size;
    // arr must be at the end so we can dynamically make it larger than
    // MIN_CACHE_SIZE by allocating extra and treating it as additional slots
    struct cache_entry arr[MIN_CACHE_SIZE];
} Cache;

Cache *Cache_create(int size, cache_lookup_cb lookup, cache_evict_cb evict);
void Cache_destroy(Cache *cache);
void *Cache_lookup(Cache *cache, void *key);
void Cache_add(Cache *cache, void *data);
void Cache_evict_object(Cache *cache, void *obj);

#endif
