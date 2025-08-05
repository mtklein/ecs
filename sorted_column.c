#include "sorted_column.h"
#include <stdlib.h>
#include <string.h>

struct sorted_column {
    struct column base;
    size_t        size;
    void         *data;
    int          *id,*ix;
    int           n,cap;
    int        (*cmp)(int, void const*, int, void const*);
};

static int max(int x, int y) {
    return x > y ? x : y;
}

static _Bool is_pow2_or_zero(int x) {
    return (x & (x-1)) == 0;
}

static void bubble(struct sorted_column *c, int ix) {
    char *tmp = malloc(c->size);
    memcpy(tmp, (char*)c->data + (size_t)ix * c->size, c->size);
    int id = c->id[ix];

    while (ix > 0) {
        int const prev = ix - 1;
        if (c->cmp(c->id[prev], (char*)c->data + (size_t)prev * c->size,
                   id, tmp) <= 0) {
            break;
        }
        memmove((char*)c->data + (size_t)ix * c->size,
                (char*)c->data + (size_t)prev * c->size, c->size);
        c->id[ix] = c->id[prev];
        c->ix[c->id[ix]] = ix;
        ix = prev;
    }
    while (ix+1 < c->n) {
        int const next = ix + 1;
        if (c->cmp(id, tmp, c->id[next],
                   (char*)c->data + (size_t)next * c->size) <= 0) {
            break;
        }
        memmove((char*)c->data + (size_t)ix * c->size,
                (char*)c->data + (size_t)next * c->size, c->size);
        c->id[ix] = c->id[next];
        c->ix[c->id[ix]] = ix;
        ix = next;
    }
    memcpy((char*)c->data + (size_t)ix * c->size, tmp, c->size);
    c->id[ix] = id;
    c->ix[id] = ix;
    free(tmp);
}

static size_t sorted_attach(struct column *base, int id, void const *src) {
    struct sorted_column *c = (struct sorted_column*)base;
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
    bubble(c, ix);
    return c->size;
}

static void sorted_detach(struct column *base, int id) {
    struct sorted_column *c = (struct sorted_column*)base;
    if (id < c->cap) {
        int ix = c->ix[id];
        if (ix >= 0) {
            int const last = --c->n;
            memmove((char      *)c->data + (size_t)ix   * c->size,
                    (char const*)c->data + (size_t)last * c->size, c->size);
            int const last_id = c->id[last];
            c->id[ix] = last_id;
            c->ix[last_id] = ix;
            c->ix[id] = ~0;
            if (ix < c->n) {
                bubble(c, ix);
            }
        }
    }
}

static size_t sorted_find(struct column const *base, int id, void *dst) {
    struct sorted_column const *c = (struct sorted_column const*)base;
    if (id < c->cap) {
        int const ix = c->ix[id];
        if (ix >= 0) {
            char const *src = (char*)c->data + (size_t)ix * c->size;
            memcpy(dst, src, c->size);
            return c->size;
        }
    }
    return 0;
}

static _Bool sorted_walk(struct column const *base, int *id) {
    struct sorted_column const *c = (struct sorted_column const*)base;

    int const ix = *id >= 0 ? c->ix[*id] : -1;
    if (ix+1 < c->n) {
        *id = c->id[ix+1];
        return 1;
    }
    return 0;
}

static void sorted_drop(struct column *base) {
    struct sorted_column *c = (struct sorted_column*)base;
    free(c->data);
    free(c->ix);
    free(c->id);
    free(c);
}

struct column* sorted_column(size_t size,
                             int (*cmp)(int, void const*, int, void const*)) {
    static struct column_vtable const vtable = {
        sorted_drop,
        sorted_attach,
        sorted_find,
        sorted_detach,
        sorted_walk,
    };

    struct sorted_column *c = calloc(1, sizeof *c);
    c->base.vptr = &vtable;
    c->size      = size;
    c->cmp       = cmp;
    return &c->base;
}
