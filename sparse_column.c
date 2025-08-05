#include "sparse_column.h"
#include <stdlib.h>
#include <string.h>

struct sparse_column {
    struct column base;
    size_t        size;
    void         *data;
    int          *id,*ix;
    int           n,cap;
};

static int max(int x, int y) {
    return x > y ? x : y;
}

static _Bool is_pow2_or_zero(int x) {
    return (x & (x-1)) == 0;
}

static void sparse_attach(struct column *base, int id, void const *src) {
    struct sparse_column *c = (struct sparse_column*)base;
    if (id >= c->cap) {
        int const grown = max(id+1, 2*c->cap);
        c->ix = realloc(c->ix, (size_t)grown * sizeof *c->ix);
        memset(c->ix + c->cap, ~0, (size_t)(grown - c->cap) * sizeof *c->ix);
        c->cap = grown;
    }

    int ix = c->ix[id];
    if (ix < 0) {
        if (is_pow2_or_zero(c->n)) {
            int const grown = c->n ? 2*c->n : 1;
            c->data = realloc(c->data, (size_t)grown * c->size);
            c->id   = realloc(c->id,   (size_t)grown * sizeof *c->id);
        }
        ix = c->n++;
        c->id[ix] = id;
        c->ix[id] = ix;
    }

    char *dst = (char*)c->data + (size_t)ix * c->size;
    memcpy(dst, src, c->size);
}

static void sparse_detach(struct column *base, int id) {
    struct sparse_column *c = (struct sparse_column*)base;
    if (id < c->cap) {
        int const ix = c->ix[id];
        if (ix >= 0) {
            int const last = --c->n;
            memmove((char      *)c->data + (size_t)ix   * c->size,
                    (char const*)c->data + (size_t)last * c->size, c->size);
            int const last_id = c->id[last];
            c->id[ix] = last_id;
            c->ix[last_id] = ix;
            c->ix[id] = ~0;
        }
    }
}

static _Bool sparse_find(struct column const *base, int id, void *dst) {
    struct sparse_column const *c = (struct sparse_column const*)base;
    if (id < c->cap) {
        int const ix = c->ix[id];
        if (ix >= 0) {
            char const *src = (char*)c->data + (size_t)ix * c->size;
            memcpy(dst, src, c->size);
            return 1;
        }
    }
    return 0;
}

static _Bool sparse_walk(struct column const *base, int *id) {
    struct sparse_column const *c = (struct sparse_column const*)base;

    int const ix = *id >= 0 ? c->ix[*id] : -1;
    if (ix+1 < c->n) {
        *id = c->id[ix+1];
        return 1;
    }
    return 0;
}

static void sparse_drop(struct column *base) {
    struct sparse_column *c = (struct sparse_column*)base;
    free(c->data);
    free(c->ix);
    free(c->id);
    free(c);
}

struct column* sparse_column(size_t size) {
    static struct column_vtable const vtable = {
        sparse_drop,
        sparse_attach,
        sparse_find,
        sparse_detach,
        sparse_walk,
    };

    struct sparse_column *c = calloc(1, sizeof *c);
    c->base.vptr = &vtable;
    c->size      = size;
    return &c->base;
}
