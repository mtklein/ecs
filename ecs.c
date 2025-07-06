#include "ecs.h"
#include <stdlib.h>
#include <string.h>

static int max(int x, int y) {
    return x>y ? x : y;
}

static _Bool is_pow2_or_zero(int x) {
    return (x & (x-1)) == 0;
}

void table_set(struct table *t, int key, void const *val) {
    if (key >= t->slots) {
        t->slots = max(key+1, 2*t->slots);
        int *ix_grown = malloc((size_t)t->slots * sizeof *ix_grown);
        for (int i = 0; i < t->slots; i++) {
            ix_grown[i] = ~0;
        }
        for (int ix = 0; ix < t->n; ix++) {
            ix_grown[t->key[ix]] = ix;
        }
        free(t->ix);
        t->ix = ix_grown;
    }

    if (is_pow2_or_zero(t->n)) {
        size_t const cap = t->n ? 2*(size_t)t->n : 1;
        size_t const bytes = cap * (sizeof *t->key + t->size);
        size_t const old_cap = t->cap;
        void *mem = realloc(t->key, bytes);
        if (mem) {
            char *base = mem;
            memmove(base + cap * sizeof *t->key,
                    base + old_cap * sizeof *t->key,
                    (size_t)t->n * t->size);
            t->key  = mem;
            t->data = base + cap * sizeof *t->key;
            t->cap  = cap;
        }
    }

    int const ix = t->n++;
    t->key[ix] = key;
    t->ix[key] = ix;
    memcpy((char*)t->data + (size_t)ix * t->size, val, t->size);
}

void table_drop(struct table *t, int key) {
    int const ix = key < t->slots ? t->ix[key] : ~0;
    if (ix != ~0) {
        t->ix[key] = ~0;
        int const back_ix  = --t->n;
        if (ix != back_ix) {
            int const back_key = t->key[back_ix];
            t->key[ix] = back_key;
            t->ix[back_key] = ix;
            memcpy((char      *)t->data + (size_t)     ix * t->size,
                   (char const*)t->data + (size_t)back_ix * t->size, t->size);
        }
    }
}

void* table_get(struct table const *t, int key) {
    if (key < t->slots) {
        int const ix = t->ix[key];
        if (ix != ~0) {
            return (char*)t->data + (size_t)ix * t->size;
        }
    }
    return NULL;
}

void table_clear(struct table *t) {
    free(t->ix);
    free(t->key);
    *t = (struct table){.size = t->size};
}
