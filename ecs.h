#pragma once

#include "array.h"

typedef struct component {
    size_t const size;
    void        *data;
    int         *id;
    int          n,cap;
} component;

int alloc_id(array *entity, array *freelist);
void drop_id(array *entity, array *freelist, int id);

void  component_set(component       *comp, int *ix, int owner, void const *val);
void  component_del(component       *comp, int *ix, int *back);
void* component_get(component const *comp, int  ix);
