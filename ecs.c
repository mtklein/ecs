#include "ecs.h"
#include <stdlib.h>
#include <string.h>

static int max(int x, int y) {
    return x > y ? x : y;
}

static _Bool is_pow2_or_zero(int x) {
    return (x & (x-1)) == 0;
}

void* component_attach(void *data, size_t size, component *comp, int id) {
    if (id >= comp->cap) {
        int const cap = max(id+1, 2*comp->cap);
        comp->ix = realloc(comp->ix, (size_t)cap * sizeof *comp->ix);
        for (int i = comp->cap; i < cap; i++) {
            comp->ix[i] = ~0;
        }
        comp->cap = cap;
    }

    if (comp->ix[id] < 0) {
        if (is_pow2_or_zero(comp->n)) {
            int const cap = comp->n ? 2*comp->n : 1;
            data     = realloc(data    , (size_t)cap * size            );
            comp->id = realloc(comp->id, (size_t)cap * sizeof *comp->id);
        }
        int const ix = comp->n++;
        comp->id[ix] = id;
        comp->ix[id] = ix;
    }

    return data;
}

void component_detach(void *data, size_t size, component *comp, int id) {
    if (id < comp->cap) {
        int const ix = comp->ix[id];
        if (ix >= 0) {
            int const last = --comp->n;
            memmove((char      *)data + (size_t)ix   * size,
                    (char const*)data + (size_t)last * size, size);
            int const last_id = comp->id[last];
            comp->id[ix] = last_id;
            comp->ix[last_id] = ix;
            comp->ix[id] = ~0;
        }
    }
}
