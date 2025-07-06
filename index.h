#pragma once

struct index {
    int *sparse;
    int *dense;
    int  vals, max_key;
};

int  index_insert(struct index       *index, int key);
void index_remove(struct index       *index, int key);
int  index_lookup(struct index const *index, int key);
