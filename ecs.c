#include "ecs.h"
#include <stdlib.h>
#include <string.h>

static _Bool is_pow2_or_zero(int x) {
    return (x & (x-1)) == 0;
}

void table_set(struct table *t, int key, void const *data) {
    if (t->max_key < key || t->n == 0) {
        t->max_key = key;

        int *sparse = malloc((size_t)(t->max_key+1) * sizeof *t->sparse);
        for (int i = 0; i <= t->max_key; i++) {
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
    memcpy((char*)t->data + (size_t)ix * t->size, data, t->size);
}

void table_drop(struct table *t, int key) {
    int const ix = key <= t->max_key ? t->sparse[key] : ~0;
    if (ix != ~0) {
        t->sparse[key] = ~0;
        int const back_ix  = --t->n,
                  back_key = t->dense[back_ix];
        if (ix != back_ix) {
            t->dense[ix] = back_key;
            t->sparse[back_key] = ix;
            memcpy((char      *)t->data + (size_t)     ix * t->size,
                   (char const*)t->data + (size_t)back_ix * t->size, t->size);
        }
    }
}

void* table_get(struct table const *t, int key) {
    if (key <= t->max_key) {
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
