#pragma once

#include <stddef.h>

struct index {
    void  *data;
    size_t elt;
    int    vals, max_key;
    int   *dense;
    int   *sparse;
};

int   index_insert(struct index       *index, int key, void const *src);
void  index_remove(struct index       *index, int key);
void* index_lookup(struct index const *index, int key);
