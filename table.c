#include "table.h"
#include <stdlib.h>
#include <string.h>

// TODO: allow for other column representations, e.g. sorted by id rather than sparse-set
struct column {
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

static void* attach(struct column *c, size_t size, int id) {
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

static void detach(struct column *c, size_t size, int id) {
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

static void* find(struct column const *c, size_t size, int id) {
    if (id < c->cap) {
        int const ix = c->ix[id];
        if (ix >= 0) {
            return (char*)c->data + (size_t)ix * size;
        }
    }
    return NULL;
}

_Bool lookup_(struct table const *t, int id, void *data, int const column[], int columns) {
    char *dst = data;
    for (int const *c = column; c < column + columns; c++) {
        size_t const size = t->column_size[*c];
        void   const *src = find(t->column + *c, size, id);
        if (!src) {
            return 0;
        }
        memcpy(dst, src, size);
        dst += size;
    }
    return 1;
}

_Bool survey_(struct table const *t, int *id, void *data, int const column[], int columns) {
    struct column const *guide = t->column + *column;
    for (int ix = *id >= 0 ? guide->ix[*id]+1 : 0; ix < guide->n; ix++) {
        *id = guide->id[ix];
        if (lookup_(t, *id, data, column, columns)) {
            return 1;
        }
    }
    return 0;
}


void update_(struct table *t, int id, void const *data, int const column[], int columns) {
    if (t->column == NULL) {
        t->column = calloc((size_t)t->columns, sizeof *t->column);
    }
    char const *src = data;
    for (int const *c = column; c < column + columns; c++) {
        size_t const size = t->column_size[*c];
        char         *dst = attach(t->column + *c, size, id);
        memcpy(dst, src, size);
        src += size;
    }
}

void erase_(struct table *t, int id, int const column[], int columns) {
    for (int const *c = column; c < column + columns; c++) {
        detach(t->column + *c, t->column_size[*c], id);
    }
}

void drop_table(struct table *t) {
    for (int i = 0; i < t->columns; i++) {
        free(t->column[i].data);
        free(t->column[i].id);
        free(t->column[i].ix);
    }
    free(t->column);
}
