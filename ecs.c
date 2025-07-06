#include "ecs.h"
#include <stdlib.h>
#include <string.h>

static _Bool is_pow2_or_zero(int x) {
    return (x & (x-1)) == 0;
}

static int max(int x, int y) {
    return x>y ? x : y;
}

void table_set(struct table *t, int key, void const *val) {
    if (t->slots <= key) {
        t->slots = max(key+1, 2*t->slots);
        int *sparse = malloc((size_t)t->slots * sizeof *t->sparse);
        for (int i = 0; i < t->slots; i++) {
            sparse[i] = ~0;
        }
        for (int ix = 0; ix < t->n; ix++) {
            sparse[t->dense[ix]] = ix;
        }
        free(t->sparse);
        t->sparse = sparse;
    }

    if (is_pow2_or_zero(t->n)) {
        size_t const cap = t->n ? 2*(size_t)t->n : 1;
        t->data  = realloc(t->data,  cap *         t->size );
        t->dense = realloc(t->dense, cap * sizeof *t->dense);
    }

    int const ix = t->n++;
    t->dense [ix] = key;
    t->sparse[key] = ix;
    memcpy((char*)t->data + (size_t)ix * t->size, val, t->size);
}

void table_drop(struct table *t, int key) {
    int const ix = key < t->slots ? t->sparse[key] : ~0;
    if (ix != ~0) {
        t->sparse[key] = ~0;
        int const back_ix  = --t->n;
        if (ix != back_ix) {
            int const back_key = t->dense[back_ix];
            t->dense[ix] = back_key;
            t->sparse[back_key] = ix;
            memcpy((char      *)t->data + (size_t)     ix * t->size,
                   (char const*)t->data + (size_t)back_ix * t->size, t->size);
        }
    }
}

void* table_get(struct table const *t, int key) {
    if (key < t->slots) {
        int const ix = t->sparse[key];
        if (ix != ~0) {
            return (char*)t->data + (size_t)ix * t->size;
        }
    }
    return NULL;
}

void table_clear(struct table *t) {
    free(t->sparse);
    free(t->dense);
    free(t->data);
    *t = (struct table){.size=t->size};
}
