#pragma once
#include <stddef.h>

typedef struct {
    int *id,*ix;
    int   n,cap;
} sparse_set;

void* component_attach(void*, size_t, int id);
void  component_detach(void*, size_t, int id);
void* component_lookup(void*, size_t, int id);
void  component_free(void*);
