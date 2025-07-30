#pragma once
#include <stddef.h>

struct component {
    void *data;
    int  *id,*ix;
    int   n,cap;
};
#define component(T) \
    union { struct {T *data; int *id,*ix; int n,cap; }; struct component type_erased; }

#define component_attach(c,id) component_attach_(&(c)->type_erased, sizeof *(c)->data, id)
#define component_detach(c,id) component_detach_(&(c)->type_erased, sizeof *(c)->data, id)
#define component_lookup(c,id) component_lookup_(&(c)->type_erased, sizeof *(c)->data, id)

void  component_attach_(struct component      *, size_t, int id);
void  component_detach_(struct component      *, size_t, int id);
void* component_lookup_(struct component const*, size_t, int id);
