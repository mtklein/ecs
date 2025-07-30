#pragma once
#include <stddef.h>

#define component(T) struct { size_t size; T *data; int *id,*ix; int n,cap; }

void  component_attach(void       *component, int id);
void  component_detach(void       *component, int id);
void* component_lookup(void const *component, int id);
