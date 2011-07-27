/*
* Based on code by Andre Reinald.
*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <mem/halloc.h>
#include "adt/radixmap.h"
#include "dbg.h"


#define ByteOf(x,y) (((uint8_t *)x)[(y)])

static inline void radix_sort(short offset, uint64_t N, uint64_t *source, uint64_t *dest)
{
    uint64_t count[256] = {0};
    uint64_t *cp = NULL;
    uint64_t *sp = NULL;
    uint64_t *end = NULL;
    uint64_t s = 0;
    uint64_t c = 0;

    // count occurences of every byte value
    for (sp = source, end = source + N; sp < end; sp++) {
        count[ByteOf(sp, offset)]++;
    }

    // transform count into index by summing elements and storing into same array
    for (s = 0, cp = count, end = count + 256; cp < end; cp++) {
        c = *cp;
        *cp = s;
        s += c;
    }

    // fill dest with the right values in the right place
    for (sp = source, end = source + N; sp < end; sp++) {
        cp = count + ByteOf(sp, offset);
        dest[*cp] = *sp;
        ++(*cp);
    }
}

void RadixMap_sort(RadixMap *map)
{
    uint64_t *source = &map->contents[0].raw;
    uint64_t *temp = &map->temp[0].raw;

    radix_sort(0, map->end, source, temp);
    radix_sort(1, map->end, temp, source);
    radix_sort(2, map->end, source, temp);
    radix_sort(3, map->end, temp, source);
}



RMElement *RadixMap_find(RadixMap *map, uint32_t to_find)
{
    int low = 0;
    int high = map->end - 1;
    RMElement *data = map->contents;

    while (low <= high) {
        int middle = low + (high - low)/2;
        uint32_t key = data[middle].data.key;

        if (to_find < key) {
            high = middle - 1;
        } else if (to_find > key) {
            low = middle + 1;
        } else {
            return &data[middle];
        }
    }

    return NULL;
}


RadixMap *RadixMap_create(size_t max)
{
    RadixMap *map = h_calloc(sizeof(RadixMap), 1);
    check_mem(map);

    map->contents = h_calloc(sizeof(RMElement), max + 1);
    check_mem(map->contents);
    hattach(map->contents, map);

    map->temp = h_calloc(sizeof(RMElement), max + 1);
    check_mem(map->temp);
    hattach(map->temp, map);

    map->max = max;
    map->end = 0;

    return map;
error:
    return NULL;
}

void RadixMap_destroy(RadixMap *map)
{
    h_free(map);
}


int RadixMap_add(RadixMap *map, uint32_t key, uint32_t value)
{
    check(key < UINT32_MAX, "Key can't be equal to UINT32_MAX.");

    RMElement element = {.data = {.key = key, .value = value}};
    check(map->end + 1 < map->max, "RadixMap is full.");

    map->contents[map->end++] = element;
    RadixMap_sort(map);

    return 0;

error:
    return -1;
}

int RadixMap_delete(RadixMap *map, RMElement *el)
{
    check(map->end > 0, "There is nothing to delete.");
    check(el != NULL, "Can't delete a NULL element.");

    el->data.key = UINT32_MAX;
    RadixMap_sort(map);
    map->end--;

    return 0;
error:
    return -1;
}


uint32_t RadixMap_push(RadixMap *map, uint32_t value)
{
    check(map->end + 1 < map->max, "RadixMap is full.");
    map->counter++;

    if(map->counter == UINT32_MAX) {
        // wrap it around so that we skip the ending trigger
        map->counter = 0;
    }

    if(map->contents[map->end-1].data.key < map->counter) {
        // this one already fits on the end so we're done
        RMElement element = {.data = {.key = map->counter, .value = value}};
        map->contents[map->end++] = element;
    } else {
        // looks like we probably wrapped around and need to do the slower add
        check(RadixMap_add(map, map->counter, value) == 0, "Failed to add on push.");
    }

    return map->counter;

error:
    return UINT32_MAX;
}
