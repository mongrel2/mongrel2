#include <adt/heap.h>
#include <stdlib.h>
#include <mem/halloc.h>
#include <assert.h>
#include <dbg.h>



inline int compare(int i, int j)
{
    return i - j;
}

inline void swap(Heap *h, int i, int j)
{
    int tmp = h->keys[i];
    h->keys[i] = h->keys[j];
    h->keys[j] = tmp;
}


inline void Heap_shift(Heap *h, int i, int max)
{
    int m = i;
    int l = 0;
    int r = 0;

    do {
        i = m;          
        l = 2 * i + 1;
        r = l + 1;

        if (l < max) {
            if (compare(h->keys[m], h->keys[l]) < 0) {
                m = l;
            }

            if (r < max && compare(h->keys[m], h->keys[r]) < 0) {
                m = r;
            }
        }

        if (m != i) {
            swap(h, i, m);
        }

    } while (m != i);
}


inline void Heap_sort(Heap *h)
{
    int k = 0;
    int max = h->i;

    for(k = max / 2; k >= 0; k--) {
        Heap_shift(h, k, max);
    }

    for(k = max - 1; k >= 0; k--) {
        swap(h, 0, k);
        Heap_shift(h, 0, k);
    }
}


void Heap_add(Heap *h, int key)
{
    assert(!Heap_is_full(h) && "The heap is full.");

    h->keys[h->i++] = key;

    // less efficient naive way, do the real version later
    Heap_sort(h);
}

void Heap_delete(Heap *h, int i)
{
    assert(!Heap_is_empty(h) && "The heap is empty.");
    assert(i < Heap_length(h) && "Heap index out of range.");

    if(i == Heap_length(h) - 1) {
        // from back is just a decrement
        h->i--;
        return;
    } else {
        // plug the back into the slot then resort
        h->i--;
        h->keys[i] = h->keys[h->i];
    }

    // naive implementation, there's a way to do this with shift
    Heap_sort(h);
}


int Heap_search(Heap *h, int key)
{
    int found = 0;
    int k = 0;
    int c = 0;
    int i = 0;
    int j = h->i;

    while (i <= j && !found) {
        k = (j + i) / 2;

        c = compare(h->keys[k], key);

        if (c == 0) {
            return k;    
        } else if (c < 0) {
            i = k + 1;
        } else {
            j = k - 1;
        }
    }

    return -1;
}



Heap *Heap_create(size_t capacity)
{
    Heap *h = (Heap *)h_calloc(sizeof(Heap), 1);
    check_mem(h);

    h->capacity = capacity;

    h->keys = h_calloc(sizeof(int), capacity);
    check_mem(h->keys);

    hattach(h, h->keys);
    return h;

error:
    Heap_destroy(h);
    return NULL;
}


void Heap_destroy(Heap *h)
{
    if(h) {
        h_free(h);
    }
}
