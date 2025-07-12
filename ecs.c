#include "ecs.h"
#include <stdlib.h>
#include <string.h>

static inline void* copy(void *dst, void const *src, size_t len) {
    return len ? memcpy(dst,src,len) : dst;
}

static int max(int x, int y) { return x>y ? x : y; }

static _Bool is_pow2_or_zero(int x) {
    return (x & (x-1)) == 0;
}

int const* iter(struct component const *c) { return c->id; }
int const* stop(struct component const *c) { return c->id + c->n; }

void attach(int id, struct component *c, void const *val) {
    for (void *dst = lookup(id,c); dst;) {
        copy(dst, val, c->size);
        return;
    }

    if (id >= c->slots) {
        int const slots = max(id+1, 2*c->slots);
        c->ix = realloc(c->ix, (size_t)slots * sizeof *c->ix);
        memset(c->ix + c->slots, ~0, (size_t)(slots - c->slots) * sizeof *c->ix);
        c->slots = slots;
    }

    if (is_pow2_or_zero(c->n)) {
        size_t const cap = c->n ? 2*(size_t)c->n : 1;
        c->id   = realloc(c->id, cap * sizeof *c->id);
        c->data = c->size ? realloc(c->data, cap * c->size) : c;
    }

    int const ix = c->n++;
    c->id[ix] = id;
    c->ix[id] = ix;
    copy((char*)c->data + (size_t)ix * c->size, val, c->size);
}

void detach(int id, struct component *c) {
    int const ix = id < c->slots ? c->ix[id] : ~0;
    if (ix != ~0) {
        c->ix[id] = ~0;
        int const back_ix  = --c->n,
                  back_id = c->id[back_ix];
        if (ix != back_ix) {
            c->id[ix] = back_id;
            c->ix[back_id] = ix;
            copy((char      *)c->data + (size_t)     ix * c->size,
                 (char const*)c->data + (size_t)back_ix * c->size, c->size);
        }
    }
}

void* lookup(int id, struct component const *c) {
    if (id < c->slots) {
        int const ix = c->ix[id];
        if (ix != ~0) {
            return (char*)c->data + (size_t)ix * c->size;
        }
    }
    return NULL;
}

void reset(struct component *c) {
    free(c->ix);
    free(c->id);
    if (c->size) {
        free(c->data);
    }
    *c = (struct component){.size=c->size};
}
