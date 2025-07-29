#include "ecs.h"
#include <stdlib.h>
#include <string.h>

typedef struct sparse_set {
    int *id,*ix;
    int   n,cap;
} sparse_set;

static sparse_set* meta_mut(void *data) {
    return data ? (void*)((char*)data - sizeof(sparse_set)) : NULL;
}

static sparse_set const* meta_const(void const *data) {
    return data ? (void const*)((char const*)data - sizeof(sparse_set)) : NULL;
}

static int max(int x, int y) {
    return x > y ? x : y;
}

static _Bool is_pow2_or_zero(int x) {
    return (x & (x-1)) == 0;
}

void* component_attach(void *data, size_t size, int id) {
    sparse_set *meta = data ? meta_mut(data) : calloc(1, sizeof *meta);
    if (id >= meta->cap) {
        int const grown = max(id+1, 2*meta->cap);
        meta->ix = realloc(meta->ix, (size_t)grown * sizeof *meta->ix);
        memset(meta->ix + meta->cap, ~0, (size_t)(grown - meta->cap) * sizeof *meta->ix);
        meta->cap = grown;
    }

    if (meta->ix[id] < 0) {
        if (is_pow2_or_zero(meta->n)) {
            int const grown = meta->n ? 2*meta->n : 1;
            meta = realloc(meta, sizeof *meta + (size_t)grown * size);
            data = (char*)meta + sizeof *meta;
            meta->id = realloc(meta->id, (size_t)grown * sizeof *meta->id);
        }
        int const ix = meta->n++;
        meta->id[ix] = id;
        meta->ix[id] = ix;
    } else {
        data = (char*)meta + sizeof *meta;
    }

    return data;
}

void component_detach(void *data, size_t size, int id) {
    sparse_set *meta = meta_mut(data);
    if (meta && id < meta->cap) {
        int const ix = meta->ix[id];
        if (ix >= 0) {
            int const last = --meta->n;
            memmove((char*)data + (size_t)ix * size,
                    (char*)data + (size_t)last * size, size);
            int const last_id = meta->id[last];
            meta->id[ix] = last_id;
            meta->ix[last_id] = ix;
            meta->ix[id] = ~0;
        }
    }
}

void* component_lookup(void *data, size_t size, int id) {
    sparse_set const *meta = meta_const(data);
    if (meta && id < meta->cap) {
        int const ix = meta->ix[id];
        if (ix >= 0) {
            return (char*)data + (size_t)ix * size;
        }
    }
    return NULL;
}

void component_free(void *data) {
    sparse_set *meta = meta_mut(data);
    if (meta) {
        free(meta->id);
        free(meta->ix);
        free(meta);
    }
}

int component_count(void const *data) {
    sparse_set const *meta = meta_const(data);
    return meta ? meta->n : 0;
}

int component_capacity(void const *data) {
    sparse_set const *meta = meta_const(data);
    return meta ? meta->cap : 0;
}

int component_index(void const *data, int id) {
    sparse_set const *meta = meta_const(data);
    return meta && id < meta->cap ? meta->ix[id] : -1;
}

int component_id_at(void const *data, int ix) {
    sparse_set const *meta = meta_const(data);
    return meta && ix < meta->n ? meta->id[ix] : -1;
}
