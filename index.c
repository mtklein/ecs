#include "index.h"
#include <stdlib.h>
#include <string.h>

static _Bool is_pow2_or_zero(int x) {
    return (x & (x-1)) == 0;
}

void index_insert(struct index *ix, int key, void const *val) {
    if (ix->max_key < key) {
        ix->max_key = key;

        int *sparse = malloc((size_t)(ix->max_key+1) * sizeof *ix->sparse);
        for (int i = 0; i <= ix->max_key; i++) {
            sparse[i] = ~0;
        }
        for (int id = 0; id < ix->vals; id++) {
            sparse[ix->dense[id]] = id;
        }
        free(ix->sparse);
        ix->sparse = sparse;
    }

    if (is_pow2_or_zero(ix->vals)) {
        size_t const cap = ix->vals ? 2*(size_t)ix->vals : 1;
        ix->data  = realloc(ix->data,  cap *         ix->elt  );
        ix->dense = realloc(ix->dense, cap * sizeof *ix->dense);
    }

    int const id = ix->vals++;
    ix->dense [ id] = key;
    ix->sparse[key] = id;
    memcpy((char*)ix->data + (size_t)id * ix->elt, val, ix->elt);
}

void index_remove(struct index *ix, int key) {
    int const id = key <= ix->max_key ? ix->sparse[key] : ~0;
    if (id != ~0) {
        ix->sparse[key] = ~0;
        int const back_id  = --ix->vals,
                  back_key = ix->dense[back_id];
        if (id != back_id) {
            ix->dense[id] = back_key;
            ix->sparse[back_key] = id;
            memcpy((char      *)ix->data + (size_t)     id * ix->elt,
                   (char const*)ix->data + (size_t)back_id * ix->elt, ix->elt);
        }
    }
}

void* index_lookup(struct index const *ix, int key) {
    if (key <= ix->max_key) {
        int const id = ix->sparse[key];
        if (id != ~0) {
            return (char*)ix->data + (size_t)id * ix->elt;
        }
    }
    return NULL;
}
