/*
* Based on code by Andre Reinald.
*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <mem/halloc.h>
#include "adt/radixmap.h"
#include "dbg.h"

// undefine this to run the more correct but slower sort
#define FAST_OPS

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

/**
 * This is primarily used by the tail sorted version to find the
 * lowest place to start sorting.  It's a quick binary search
 * of the data elements and returns whatever lowest value was
 * hit.
 */
RMElement *RadixMap_find_lowest(RadixMap *map, uint32_t to_find)
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

    return &data[low];
}

/**
 * A special version useful for things like add and delete that
 * takes a "hint", and then avoid sorting the whole array depending
 * on where the hint might fall.
 */
static inline void RadixMap_sort_tail(RadixMap *map, RMElement *hint)
{
    uint64_t *source = &map->contents[0].raw;
    uint64_t *temp = &map->temp[0].raw;
    size_t count = map->end;
    uint32_t max = 0;

    // if we only have to sort the top part then only do that
    if(count > 2) {
        if(hint->data.key == UINT32_MAX) {
            // this is a delete, so the hint is moving to the end,
            // only sort from the hint on
            source = &hint->raw;
            // we can just swap out the last one and drop the end by one
            *hint = map->contents[map->end - 1];
            count = map->contents + map->end - hint - 1;

            // that used to be the max
            max = hint->data.key;
        } else {
            // looks like this one is an add at the end
            // this is a simple optimization that gets decent performance
            RMElement *middle = RadixMap_find_lowest(map, hint->data.key);

            // middle is the bottom of the possible range
            count = map->contents + map->end - middle;
            source = &middle->raw;

            max = map->contents[map->end - 1].data.key;
        }

        // always have to sort the first two bytes worth
        radix_sort(0, count, source, temp);
        radix_sort(1, count, temp, source);

        if(max > UINT16_MAX) {
            // only sort if the max possible number is outside the first 2 bytes
            radix_sort(2, count, source, temp);
            radix_sort(3, count, temp, source);
        }
    } else {
        // shouldn't be a super common case, but if there's only
        // 2 elements then just a single comparison is best
        if(map->contents[0].data.key > map->contents[1].data.key) {
            map->temp[0] = map->contents[0];
            map->contents[0] = map->contents[1];
            map->contents[1] = map->temp[0];
        } else {
            // pass, there's only 2 elements, and they're already sorted
        }
    }
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
    RadixMap *map = NULL;
    map = calloc(sizeof(RadixMap), 1);

    check_mem(map);

    map->contents = calloc(sizeof(RMElement), max + 1);
    check_mem(map->contents);

    map->temp = calloc(sizeof(RMElement), max + 1);
    check_mem(map->temp);

    map->max = max;
    map->end = 0;

    return map;
error:
    if(map) {
        if(map->contents) {
            free(map->contents);
        }
        if(map->temp) {
            free(map->temp);
        }
        free(map);
    }
    return NULL;
}

void RadixMap_destroy(RadixMap *map)
{
    if(map) {
        free(map->contents);
        free(map->temp);
        free(map);
    }
}


int RadixMap_add(RadixMap *map, uint32_t key, uint32_t value)
{
    check(key < UINT32_MAX, "Key can't be equal to UINT32_MAX.");

    RMElement element = {.data = {.key = key, .value = value}};
    check(map->end + 1 < map->max, "RadixMap is full.");

    map->contents[map->end++] = element;

#ifdef FAST_OPS
    RadixMap_sort_tail(map, map->contents + map->end - 1);
#else
    RadixMap_sort(map);
#endif

    return 0;

error:
    return -1;
}

int RadixMap_delete(RadixMap *map, RMElement *el)
{
    check(map->end > 0, "There is nothing to delete.");
    check(el != NULL, "Can't delete a NULL element.");

    el->data.key = UINT32_MAX;

    if(map->end > 1) {
        // don't bother resorting a map of 1 length
#ifdef FAST_OPS
        RadixMap_sort_tail(map, el);
#else
        RadixMap_sort(map);
#endif
    }

    map->end--;

    return 0;
error:
    return -1;
}


uint32_t RadixMap_push(RadixMap *map, uint32_t value)
{
    RMElement *found = NULL;

    check(map->end + 1 < map->max, "RadixMap is full.");

    do {
        map->counter++;

        if(map->counter == UINT32_MAX) {
            // wrap it around so that we skip the ending trigger
            map->counter = 0;
        }

        found = RadixMap_find(map, map->counter);
    } while (found);

    if(map->end == 0 || map->contents[map->end-1].data.key < map->counter) {
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
