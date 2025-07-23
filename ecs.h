#pragma once

#include "array.h"

int alloc_id(array *entity, array *freelist);
void drop_id(array *entity, array *freelist, int id);

void  component_set(array       *comp, int *ix, void const *val);
void  component_del(array       *comp, int *ix, int *back);
void* component_get(array const *comp, int  ix);
