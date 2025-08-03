#pragma once

#include "len.h"
#include <stddef.h>

struct table {
    size_t const     *column_size;
    int               columns;
    int               :32;
    struct component *comp;
};
void drop_table(struct table *);

_Bool lookup_(struct table const*, int  id, void       *data, int const columns[], int n);
_Bool survey_(struct table const*, int *id, void       *data, int const columns[], int n);
void  update_(struct table      *, int  id, void const *data, int const columns[], int n);

#define lookup(t,id,data,...) \
    lookup_((t),(id),(data),(int const[]){__VA_ARGS__}, len((int const[]){__VA_ARGS__}))
#define survey(t,id,data,...) \
    survey_((t),(id),(data),(int const[]){__VA_ARGS__}, len((int const[]){__VA_ARGS__}))
#define update(t,id,data,...) \
    update_((t),(id),(data),(int const[]){__VA_ARGS__}, len((int const[]){__VA_ARGS__}))
