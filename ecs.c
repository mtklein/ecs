#include "ecs.h"
#include <stdlib.h>
#include <string.h>

static struct component *components;

static void component_register(struct component *c, size_t size) {
    if (!c->size) {
        c->size = size;
        c->next = components;
        components = c;
    }
}

static int max(int x, int y) {
    return x > y ? x : y;
}

static _Bool is_pow2_or_zero(int x) {
    return (x & (x-1)) == 0;
}

void* component_attach_(struct component *c, size_t size, int id) {
    component_register(c, size);
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

void component_detach_(struct component *c, size_t size, int id) {
    component_register(c, size);
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

void* component_lookup_(struct component *c, size_t size, int id) {
    component_register(c, size);
    if (id < c->cap) {
        int const ix = c->ix[id];
        if (ix >= 0) {
            return (char*)c->data + (size_t)ix * size;
        }
    }
    return NULL;
}

void component_free_(struct component *c) {
    free(c->data);
    free(c->id);
    free(c->ix);

    struct component **p = &components;
    while (*p) {
        if (*p == c) {
            *p = c->next;
            break;
        }
        p = &(*p)->next;
    }

    c->data = NULL;
    c->id = NULL;
    c->ix = NULL;
    c->next = NULL;
    c->size = 0;
    c->n = 0;
    c->cap = 0;
}

void entity_drop(int id) {
    struct component *c = components;
    while (c) {
        component_detach_(c, c->size, id);
        c = c->next;
    }
}
