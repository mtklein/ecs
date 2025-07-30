#pragma once
#include <stddef.h>

struct component {
    void  *data;
    int   *id,*ix;
    int    n,cap;
    size_t size;
};

void* component_attach(struct component *, int id);
void  component_detach(struct component *, int id);
void* component_lookup(struct component const*, int id);
