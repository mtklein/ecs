#pragma once

#include <stddef.h>

struct tree_node;

struct tree {
    size_t           size;
    int              cap;
    int              n;
    struct tree_node *root;
};

void tree_reset(struct tree*);
void tree_attach(int id, struct tree*, void const *val);
void* tree_lookup(int id, struct tree const*);
void tree_detach(int id, struct tree*);
