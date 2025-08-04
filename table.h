#pragma once

#include "len.h"
#include <stddef.h>

struct table {
    struct column *column;
    size_t const  *column_size;
    int            columns;
    int            :32;
};
void drop_table(struct table*);

_Bool lookup_(struct table const*, int  id, void       *data, int const column[], int columns);
_Bool survey_(struct table const*, int *id, void       *data, int const column[], int columns);
void  update_(struct table      *, int  id, void const *data, int const column[], int columns);
void   erase_(struct table      *, int  id                  , int const column[], int columns);

#define lookup(t,id,data, ...) \
    lookup_(t,id,data, (int const[]){__VA_ARGS__}, len(((int const[]){__VA_ARGS__})))
#define survey(t,id,data, ...) \
    survey_(t,id,data, (int const[]){__VA_ARGS__}, len(((int const[]){__VA_ARGS__})))
#define update(t,id,data, ...) \
    update_(t,id,data, (int const[]){__VA_ARGS__}, len(((int const[]){__VA_ARGS__})))
#define erase(t,id, ...) \
    erase_(t,id      , (int const[]){__VA_ARGS__}, len(((int const[]){__VA_ARGS__})))