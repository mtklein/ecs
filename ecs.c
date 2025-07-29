#include "ecs.h"
#include <stdlib.h>
#include <string.h>

static int max(int x, int y) {
    return x > y ? x : y;
}

static _Bool is_pow2_or_zero(int x) {
    return (x & (x-1)) == 0;
}

void* component_attach(void *data, size_t size, sparse_set *meta, int id) {
    if (id >= meta->cap) {
        int const grown = max(id+1, 2*meta->cap);
        meta->ix = realloc(meta->ix, (size_t)grown * sizeof *meta->ix);
        memset(meta->ix + meta->cap, ~0, (size_t)(grown - meta->cap) * sizeof *meta->ix);
        meta->cap = grown;
    }

    if (meta->ix[id] < 0) {
        if (is_pow2_or_zero(meta->n)) {
            int const grown = meta->n ? 2*meta->n : 1;
            data     = realloc(data    , (size_t)grown * size            );
            meta->id = realloc(meta->id, (size_t)grown * sizeof *meta->id);
        }
        int const ix = meta->n++;
        meta->id[ix] = id;
        meta->ix[id] = ix;
    }

    return data;
}

void component_detach(void *data, size_t size, sparse_set *meta, int id) {
    if (id < meta->cap) {
        int const ix = meta->ix[id];
        if (ix >= 0) {
            int const last = --meta->n;
            memmove((char      *)data + (size_t)ix   * size,
                    (char const*)data + (size_t)last * size, size);
            int const last_id = meta->id[last];
            meta->id[ix] = last_id;
            meta->ix[last_id] = ix;
            meta->ix[id] = ~0;
        }
    }
}

void* component_lookup(void *data, size_t size, sparse_set const *meta, int id) {
    if (id < meta->cap) {
        int const ix = meta->ix[id];
        if (ix >= 0) {
            return (char*)data + (size_t)ix * size;
        }
    }
    return NULL;
}
