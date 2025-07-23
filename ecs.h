#pragma once
#include <stddef.h>

typedef struct {
    int *id,*ix;
    int   n,cap;
} component;

void* component_attach(void *data, size_t size, component*, int id);
void  component_detach(void *data, size_t size, component*, int id);
