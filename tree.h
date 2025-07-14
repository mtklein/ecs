#pragma once

#include <stddef.h>

struct tree {
    size_t size;
    void  *data;
    int   *id;
    int   *left,*right,*parent;
    int    n,cap,root;
    int    :32;
};

void tree_reset(struct tree*);
void tree_attach(int id, struct tree*, void const *val);
void* tree_lookup(int id, struct tree const*);
void tree_detach(int id, struct tree*);
