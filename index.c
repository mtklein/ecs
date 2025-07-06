#include "index.h"
#include <stdlib.h>
#include <string.h>

static _Bool is_pow2_or_zero(int x) {
    return (x & (x-1)) == 0;
}

int index_insert(struct index *ix, int key) {
    if (ix->max_key < key) {
        ix->max_key = key;

        int *sparse = malloc((size_t)(ix->max_key+1) * sizeof *ix->sparse);
        for (int i = 0; i <= ix->max_key; i++) {
            sparse[i] = ~0;
        }
        for (int val = 0; val < ix->vals; val++) {
            sparse[ix->dense[val]] = val;
        }
        free(ix->sparse);
        ix->sparse = sparse;
    }

    if (is_pow2_or_zero(ix->vals)) {
        size_t cap = ix->vals ? 2u*(size_t)ix->vals : 1u;
        ix->dense = realloc(ix->dense, cap * sizeof *ix->dense);
        ix->data  = realloc(ix->data,  cap * ix->elt);
    }

    int const val = ix->vals++;
    ix->dense[val] = key;
    ix->sparse[key] = val;
    return val;
}

void index_remove(struct index *ix, int key) {
    int const val = index_lookup(ix, key);
    if (val != ~0) {
        ix->sparse[key] = ~0;
        int const back_val = --ix->vals,
                  back_key = ix->dense[back_val];
        if (val != back_val) {
            ix->dense[val] = back_key;
            ix->sparse[back_key] = val;
            if (ix->elt) {
                char *data = ix->data;
                size_t const off = (size_t)val * ix->elt,
                               back_off = (size_t)back_val * ix->elt;
                memcpy(data + off, data + back_off, ix->elt);
            }
        }
    }
}

int index_lookup(struct index const *ix, int key) {
    return key <= ix->max_key ? ix->sparse[key] : ~0;
}
