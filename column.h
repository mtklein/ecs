#pragma once
#include <stddef.h>

struct column_vtable;

struct column {
    struct column_vtable const *vptr;
};

struct column_vtable {
    void   (*  drop)(struct column      *);
    size_t (*attach)(struct column      *, int  id, void const*);
    size_t (*  find)(struct column const*, int  id, void      *);
    void   (*detach)(struct column      *, int  id);
    _Bool  (*  walk)(struct column const*, int *id);
};
