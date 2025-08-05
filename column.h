#pragma once

struct column_vtable;

struct column {
    struct column_vtable const *vptr;
};

struct column_vtable {
    void  (*  drop)(struct column      *);
    void  (*attach)(struct column      *, int  id, void const*);
    _Bool (*  find)(struct column const*, int  id, void      *);
    void  (*detach)(struct column      *, int  id);
    _Bool (*  walk)(struct column const*, int *id);
};
