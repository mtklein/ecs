#pragma once

#include <stddef.h>

struct component {
    size_t size;
    void  *data;
    int    n,slots;
    int   *id,*ix;
};
void reset(struct component*);

void  attach(int id, struct component*, void const *val);
void  detach(int id, struct component*);
void* lookup(int id, struct component const *);
