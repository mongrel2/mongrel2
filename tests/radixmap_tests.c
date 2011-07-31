#include "minunit.h"
#include <adt/radixmap.h>
#include <time.h>

static int make_random(RadixMap *map)
{
    size_t i = 0;

    for (i = 0; i < map->max - 1; i++) {
        uint32_t key = (uint32_t)(rand () | (rand () << 16));
        check(RadixMap_add(map, key, i) == 0, "Failed to add key %u.", key);
    }

    return i;

error:
    return 0;
}

static int push_random_values(RadixMap *map)
{
    size_t i = 0;
    for(i = 0; i < map->max - 1; i++) {
        uint32_t value = (uint32_t)(rand () | (rand () << 16));
        uint32_t key = RadixMap_push(map, value);
        check(key != UINT32_MAX, "Got an invalid return.");
    }
    return 1;
error:
    return 0;
}

static int check_order(RadixMap *map)
{
    RMElement d1, d2;
    size_t i = 0;

    // only signal errors if any (should not be)
    for (i = 0; map->end > 0 && i < map->end-1; i++) {
        d1 = map->contents[i];
        d2 = map->contents[i+1];

        if(d1.data.key > d2.data.key) {
            debug("FAIL:i=%u, key: %u, value: %u, equals max? %d\n", i, d1.data.key, d1.data.value,
                    d2.data.key == UINT32_MAX);
            return 0;
        }
    }

    return 1;
}

static int test_search(RadixMap *map)
{
    size_t i = 0;
    RMElement *d = NULL;
    RMElement *found = NULL;

    for(i = map->end / 2; i < map->end; i++) {
        d = &map->contents[i];
        found = RadixMap_find(map, d->data.key);
        check(found != NULL, "Didn't find %u at %u.", d->data.key, i);
        check(found->data.key == d->data.key, "Got the wrong result: %p:%u looking for %u at %u", 
                found, found->data.key, d->data.key, i);
    }

    return 1;
error:
    return 0;
}

// test for big number of elements
static char *test_RadixMap_operations()
{
    size_t N = 200;

    RadixMap *map = RadixMap_create(N);
    mu_assert(map != NULL, "Failed to make the map.");
    mu_assert(make_random(map), "Didn't make a random fake radix map.");

    RadixMap_sort(map);
    mu_assert(check_order(map), "Failed to properly sort the RadixMap.");

    mu_assert(test_search(map), "Failed the search test.");
    mu_assert(check_order(map), "RadixMap didn't stay sorted after search.");

    RadixMap_destroy(map);

    map = RadixMap_create(N);
    mu_assert(map != NULL, "Failed to make the map.");

    debug("PUSHING VALUES");
    mu_assert(push_random_values(map), "Didn't push random values.");
    debug("VALUES PUSHED!");

    mu_assert(check_order(map), "Map wasn't sorted after pushes.");

    debug("DOING DELETES");
    while(map->end > 0) {
        RMElement *el = RadixMap_find(map, map->contents[map->end / 2].data.key);
        mu_assert(el != NULL, "Should get a result.");

        size_t old_end = map->end;

        mu_assert(RadixMap_delete(map, el) == 0, "Didn't delete it.");
        mu_assert(old_end - 1 == map->end, "Wrong size after delete.");

        // test that the end is now the old value, but uint32 max so it trails off
        mu_assert(check_order(map), "RadixMap didn't stay sorted after delete.");
    }

    RadixMap_destroy(map);

    return NULL;
}


static char *test_RadixMap_simulate()
{
   uint32_t fd = (uint32_t)(rand() | (rand() << 16));
   int i = 0;
   size_t key = 0;
   int connects = 0, disconnects = 0, activities = 0;
   RMElement *el = NULL;
   RadixMap *map = RadixMap_create(50000);
   // start the counter at just near max to test that out
   map->counter = UINT32_MAX - 100;

   debug("COUNTER: %u", map->counter);

   for(i = 0; i < 10; i++) {
       // seed it up
       RadixMap_push(map, fd++);
   }

   debug("ENTERING LOOP");
   for(i = 0; i < 1000000; i++) {
       switch((rand() + 1) % 4) {
           case 0:
               // connect, so add it on
               RadixMap_push(map, fd++);
               connects++;
               break;
           case 1:
               // disconnect, so remove
               if(map->end > 0) {
                   key = (rand() + 1) % map->end;
                   el = &map->contents[key];
                   el = RadixMap_find(map, el->data.key);
                   mu_assert(el != NULL, "Failed to find the key.");
                   RadixMap_delete(map, el);
               }
               disconnects++;
               break;

           default:
               // activity so find for doing stuff
               if(map->end > 0) {
                   key = (rand() + 1) % map->end;
                   el = &map->contents[key];
                   el = RadixMap_find(map, el->data.key);
                   mu_assert(el != NULL, "Failed to find the key.");
               }
               activities++;
               break;
       }

       if(map->end == 0) {
           // make a new connect if we're empty
           RadixMap_push(map, fd++);
       }
   }

   RadixMap_destroy(map);
   return NULL;
}

char *all_tests() 
{
    mu_suite_start();
    srand(time(NULL));

    mu_run_test(test_RadixMap_operations);
    mu_run_test(test_RadixMap_simulate);

    return NULL;
}

RUN_TESTS(all_tests);

