#include "table.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

struct component {
    void *data;
    int  *id,*ix;
    int   n,cap;
};

static int max(int x, int y) {
    return x > y ? x : y;
}

static _Bool is_pow2_or_zero(int x) {
    return (x & (x-1)) == 0;
}

static void* component_attach_(struct component *c, size_t size, int id) {
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
            c->data = realloc(c->data, (size_t)grown * size);
            c->id   = realloc(c->id,   (size_t)grown * sizeof *c->id);
        }
        ix = c->n++;
        c->id[ix] = id;
        c->ix[id] = ix;
    }

    return (char*)c->data + (size_t)ix * size;
}

static void* component_lookup_(struct component const *c, size_t size, int id) {
    if (id < c->cap) {
        int const ix = c->ix[id];
        if (ix >= 0) {
            return (char*)c->data + (size_t)ix * size;
        }
    }
    return NULL;
}

static void ensure(struct table *t) {
    if (!t->comp && t->columns) {
        t->comp = calloc((size_t)t->columns, sizeof *t->comp);
    }
}

_Bool lookup_(struct table const *t, int id, void *data, int const cols[], int n) {
    if (!t->comp) {
        return 0;
    }

    size_t off = 0;
    for (int i = 0; i < n; i++) {
        int const col = cols[i];
        void const *src = component_lookup_(t->comp + col, t->column_size[col], id);
        if (!src) {
            return 0;
        }
        memcpy((char*)data + off, src, t->column_size[col]);
        off += t->column_size[col];
    }
    return off > 0;
}

_Bool survey_(struct table const *t, int *id, void *data, int const cols[], int n) {
    if (!t->comp || !n) {
        return 0;
    }

    struct component const *base = t->comp + cols[0];
    int const last = *id;
    int next = INT_MAX;
    for (int i = 0; i < base->n; i++) {
        int const ent = base->id[i];
        if (ent <= last || ent >= next) {
            continue;
        }
        _Bool ok = 1;
        for (int j = 1; j < n; j++) {
            if (!component_lookup_(t->comp + cols[j], t->column_size[cols[j]], ent)) {
                ok = 0;
                break;
            }
        }
        if (ok) {
            next = ent;
        }
    }
    if (next == INT_MAX) {
        return 0;
    }

    size_t off = 0;
    for (int j = 0; j < n; j++) {
        void const *src = component_lookup_(t->comp + cols[j], t->column_size[cols[j]], next);
        memcpy((char*)data + off, src, t->column_size[cols[j]]);
        off += t->column_size[cols[j]];
    }
    *id = next;
    return 1;
}

void update_(struct table *t, int id, void const *data, int const cols[], int n) {
    ensure(t);
    size_t off = 0;
    for (int i = 0; i < n; i++) {
        int const col = cols[i];
        size_t const size = t->column_size[col];
        void *dst = component_attach_(t->comp + col, size, id);
        memcpy(dst, (char const*)data + off, size);
        off += size;
    }
}

static void component_detach_(struct component *c, size_t size, int id) {
    if (id < c->cap) {
        int const ix = c->ix[id];
        if (ix >= 0) {
            int const last = --c->n;
            memmove((char      *)c->data + (size_t)ix   * size,
                    (char const*)c->data + (size_t)last * size, size);
            int const last_id = c->id[last];
            c->id[ix] = last_id;
            c->ix[last_id] = ix;
            c->ix[id] = ~0;
        }
    }
}

void erase_(struct table *t, int id, int const cols[], int n) {
    if (!t->comp) {
        return;
    }
    for (int i = 0; i < n; i++) {
        int const col = cols[i];
        component_detach_(t->comp + col, t->column_size[col], id);
    }
}

void drop_table(struct table *t) {
    if (t->comp) {
        for (int i = 0; i < t->columns; i++) {
            free(t->comp[i].data);
            free(t->comp[i].id);
            free(t->comp[i].ix);
        }
        free(t->comp);
        t->comp = NULL;
    }
}
