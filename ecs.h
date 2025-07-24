#pragma once
#include <stddef.h>

typedef struct {
    int *id,*ix;
    int   n,cap;
} sparse_set;

void* component_attach(void *data, size_t size, sparse_set*, int id);
void  component_detach(void *data, size_t size, sparse_set*, int id);
