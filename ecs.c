#include "ecs.h"
#include <stdlib.h>
#include <string.h>

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
    if (t->size) {
        memcpy((char*)t->data + (size_t)ix * t->size, val, t->size);
    }
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

void table_join(struct table const *a, struct table const *b,
                join_cb *cb, void *ctx) {
    struct table const *small = a->n <= b->n ? a : b;
    struct table const *large = a->n <= b->n ? b : a;
    _Bool const small_is_a = small == a;

    for (int i = 0; i < small->n; i++) {
        int const key = small->key[i];
        int const ix  = key < large->slots ? large->ix[key] : ~0;
        if (ix != ~0) {
            void const *a_val = small_is_a
                                ? (char const*)a->data + (size_t)i  * a->size
                                : (char const*)a->data + (size_t)ix * a->size;
            void const *b_val = small_is_a
                                ? (char const*)b->data + (size_t)ix * b->size
                                : (char const*)b->data + (size_t)i  * b->size;
            cb(key, a_val, b_val, ctx);
        }
    }
}

void table_join_many(struct table const **tables, int count,
                     join_many_cb *cb, void *ctx) {
    int small_ix = 0;
    for (int i = 1; i < count; i++) {
        if (tables[i]->n < tables[small_ix]->n) {
            small_ix = i;
        }
    }

    void const **vals = malloc((size_t)count * sizeof *vals);
    struct table const *small = tables[small_ix];

    for (int i = 0; i < small->n; i++) {
        int const key = small->key[i];
        _Bool ok = 1;
        for (int j = 0; j < count; j++) {
            struct table const *t = tables[j];
            int ix = j == small_ix ? i : (key < t->slots ? t->ix[key] : ~0);
            if (ix == ~0) {
                ok = 0;
                break;
            }
            vals[j] = t->size ? (char const*)t->data + (size_t)ix * t->size : NULL;
        }
        if (ok) {
            cb(key, vals, ctx);
        }
    }

    free(vals);
}
