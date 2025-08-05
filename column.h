#pragma once

struct column {
    struct column_vtable {
        void  (*  drop)(struct column      *);
        void  (*attach)(struct column      *, int  id, void const*);
        _Bool (*  find)(struct column const*, int  id, void      *);
        void  (*detach)(struct column      *, int  id);
        _Bool (*  walk)(struct column const*, int *id);
    } const *vptr;
};

