#include "index.h"
#include <stdlib.h>

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
        ix->dense = realloc(ix->dense, (size_t)(ix->vals ? 2*ix->vals : 1) * sizeof *ix->dense);
    }

    int const val = ix->vals++;
    ix->dense [val] = key;
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
        }
    }
}

int index_lookup(struct index const *ix, int key) {
    return key <= ix->max_key ? ix->sparse[key] : ~0;
}
