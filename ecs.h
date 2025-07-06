#pragma once

#include <stddef.h>

struct component {
    size_t size;
    void  *data;
    int    n, max;
    int   *dense;
    int   *sparse;
};

void  component_attach(struct component      *, int entity, void const *data);
void  component_detach(struct component      *, int entity);
void* component_lookup(struct component const*, int entity);
void  component_free  (struct component*);
