#include "ecs.h"
#include <stdlib.h>
#include <string.h>

static _Bool is_pow2_or_zero(int x) {
    return (x & (x-1)) == 0;
}

void component_attach(struct component *c, int entity, void const *data) {
    if (c->max < entity || c->n == 0) {
        c->max = entity;

        int *sparse = malloc((size_t)(c->max+1) * sizeof *c->sparse);
        for (int i = 0; i <= c->max; i++) {
            sparse[i] = ~0;
        }
        for (int ix = 0; ix < c->n; ix++) {
            sparse[c->dense[ix]] = ix;
        }
        free(c->sparse);
        c->sparse = sparse;
    }

    if (is_pow2_or_zero(c->n)) {
        size_t const cap = c->n ? 2*(size_t)c->n : 1;
        c->data  = realloc(c->data,  cap *         c->size );
        c->dense = realloc(c->dense, cap * sizeof *c->dense);
    }

    int const ix = c->n++;
    c->dense [ix] = entity;
    c->sparse[entity] = ix;
    memcpy((char*)c->data + (size_t)ix * c->size, data, c->size);
}

void component_detach(struct component *c, int entity) {
    int const ix = entity <= c->max ? c->sparse[entity] : ~0;
    if (ix != ~0) {
        c->sparse[entity] = ~0;
        int const back_ix  = --c->n,
                  back_key = c->dense[back_ix];
        if (ix != back_ix) {
            c->dense[ix] = back_key;
            c->sparse[back_key] = ix;
            memcpy((char      *)c->data + (size_t)     ix * c->size,
                   (char const*)c->data + (size_t)back_ix * c->size, c->size);
        }
    }
}

void* component_lookup(struct component const *c, int entity) {
    if (entity <= c->max) {
        int const ix = c->sparse[entity];
        if (ix != ~0) {
            return (char*)c->data + (size_t)ix * c->size;
        }
    }
    return NULL;
}

void component_free(struct component *c) {
    free(c->sparse);
    free(c->dense);
    free(c->data);
}
