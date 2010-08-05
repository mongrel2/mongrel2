#ifndef _heap_h
#define _heap_h

#include <stdlib.h>

typedef struct Heap {
    int i;
    size_t capacity;
    int *keys;
} Heap;


#define Heap_length(H) ((H)->i)

#define Heap_capacity(H) ((H)->capacity)

#define Heap_is_empty(H) (Heap_length(H) == 0)

#define Heap_is_full(H) (Heap_length(H) == Heap_capacity(H))

#define Heap_at(H, i) ((H)->keys[(i)])

#define Heap_first(H) Heap_at(H, 0)

void Heap_destroy(Heap *h);

Heap *Heap_init(size_t dim);

void Heap_add(Heap *h, int key);

void Heap_delete(Heap *h, int i);

int Heap_search(Heap *h, int key);

#endif
