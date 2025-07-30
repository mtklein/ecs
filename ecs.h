#pragma once
#include <stddef.h>

#define component(T) struct { T *data; int *id,*ix; int n,cap; }

#define component_attach(c,id) component_attach_(c, sizeof *(c)->data, id)
#define component_detach(c,id) component_detach_(c, sizeof *(c)->data, id)
#define component_lookup(c,id) component_lookup_(c, sizeof *(c)->data, id)

void  component_attach_(void       *component, size_t, int id);
void  component_detach_(void       *component, size_t, int id);
void* component_lookup_(void const *component, size_t, int id);
