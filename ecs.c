#include "ecs.h"
#include <stdlib.h>
#include <string.h>

static inline void* careful_memcpy(void *dst, void const *src, size_t len) {
    return len ? memcpy(dst,src,len) : dst;
}
#if defined(memcpy)
    #undef  memcpy
#endif
#define memcpy careful_memcpy

static int max(int x, int y) { return x>y ? x : y; }

static _Bool is_pow2_or_zero(int x) {
    return (x & (x-1)) == 0;
}

void table_set(struct table *t, int key, void const *val) {
    if (key >= t->slots) {
        int const slots = max(key+1, 2*t->slots);
        t->ix = realloc(t->ix, (size_t)slots * sizeof *t->ix);
        memset(t->ix + t->slots, ~0, (size_t)(slots - t->slots) * sizeof *t->ix);
        t->slots = slots;
    }

    if (is_pow2_or_zero(t->n)) {
        size_t const cap = t->n ? 2*(size_t)t->n : 1;
        t->key  = realloc(t-> key, cap * sizeof *t->key);
        t->data = t->size ? realloc(t->data, cap * t->size) : t;
    }

    int const ix = t->n++;
    t->key[ix] = key;
    t->ix[key] = ix;
    memcpy((char*)t->data + (size_t)ix * t->size, val, t->size);
}

void table_del(struct table *t, int key) {
    int const ix = key < t->slots ? t->ix[key] : ~0;
    if (ix != ~0) {
        t->ix[key] = ~0;
        int const back_ix  = --t->n,
                  back_key = t->key[back_ix];
        if (ix != back_ix) {
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

void table_reset(struct table *t) {
    free(t->ix);
    free(t->key);
    if (t->size) {
        free(t->data);
    }
    *t = (struct table){.size=t->size};
}

_Bool table_join(struct table const *table[], int tables, int *key, void *vals) {
    struct table const *lead = table[0];
    for (int ix = *key >= 0 ? 1+lead->ix[*key] : 0; ix < lead->n; ix++) {
        *key = lead->key[ix];
        char *dst = vals;
        for (int i = 0; i < tables; i++) {
            void const *src = table_get(table[i], *key);
            if (!src) {
                goto next_ix;
            }
            memcpy(dst, src, table[i]->size);
            dst += table[i]->size;
        }
        return 1;

    next_ix: continue;
    }
    return 0;
}
