#pragma once
#include <stddef.h>

typedef struct sparse_set {
    int  *sparse,
         *dense;
    int   n,
          cap,
          sparse_cap,
          :32;
} sparse_set;

void sparse_set_attach(sparse_set *set, void **data, size_t size, int id, void const *val);
void sparse_set_detach(sparse_set *set, void **data, size_t size, int id);

static inline int sparse_set_index(sparse_set const *set, int id) {
    return id < set->sparse_cap ? set->sparse[id] : -1;
}
