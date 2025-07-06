#pragma once

#include <stddef.h>

struct index {
    int   *sparse;
    int   *dense;
    void  *data;
    size_t elt;
    int    vals, max_key;
};

int  index_insert(struct index       *index, int key);
void index_remove(struct index       *index, int key);
int  index_lookup(struct index const *index, int key);
