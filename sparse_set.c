#include "sparse_set.h"
#include <stdlib.h>
#include <string.h>

static void grow_sparse(sparse_set *set, int want) {
    if (want <= set->sparse_cap) {
        return;
    }
    int new_cap = set->sparse_cap ? set->sparse_cap : 1;
    while (new_cap <= want) {
        new_cap *= 2;
    }
    set->sparse = realloc(set->sparse, (size_t)new_cap * sizeof *set->sparse);
    for (int i = set->sparse_cap; i < new_cap; i++) {
        set->sparse[i] = -1;
    }
    set->sparse_cap = new_cap;
}

static void grow_dense(sparse_set *set, void **data, size_t size) {
    if (set->n < set->cap) {
        return;
    }
    int new_cap = set->cap ? set->cap * 2 : 1;
    set->dense = realloc(set->dense, (size_t)new_cap * sizeof *set->dense);
    *data = realloc(*data, (size_t)new_cap * size);
    set->cap = new_cap;
}

void sparse_set_attach(sparse_set *set, void **data, size_t size, int id, void const *val) {
    grow_sparse(set, id+1);
    int ix = set->sparse[id];
    if (ix >= 0) {
        memcpy((char*)*data + (size_t)ix * size, val, size);
        return;
    }
    grow_dense(set, data, size);
    ix = set->n++;
    set->dense[ix] = id;
    set->sparse[id] = ix;
    memcpy((char*)*data + (size_t)ix * size, val, size);
}

void sparse_set_detach(sparse_set *set, void **data, size_t size, int id) {
    if (id >= set->sparse_cap) {
        return;
    }
    int ix = set->sparse[id];
    if (ix < 0) {
        return;
    }
    int last = --set->n;
    if (ix != last) {
        memcpy((char*)*data + (size_t)ix * size,
               (char*)*data + (size_t)last * size,
               size);
        int last_id = set->dense[last];
        set->dense[ix] = last_id;
        set->sparse[last_id] = ix;
    }
    set->sparse[id] = -1;
}
