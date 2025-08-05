#pragma once

#include "column.h"
#include "len.h"

_Bool lookup_(int  id, void       *data, struct column const**, int);
_Bool survey_(int *id, void       *data, struct column const**, int);
void  update_(int  id, void const *data, struct column      **, int);
void   erase_(int  id                  , struct column      **, int);

#define lookup(id, data, ...) \
    lookup_(id, data, (struct column const*[]){__VA_ARGS__}, \
                 len(((struct column const*[]){__VA_ARGS__})))

#define survey(id, data, ...) \
    survey_(id, data, (struct column const*[]){__VA_ARGS__}, \
                 len(((struct column const*[]){__VA_ARGS__})))

#define update(id, data, ...) \
    update_(id, data, (struct column      *[]){__VA_ARGS__}, \
                 len(((struct column      *[]){__VA_ARGS__})))

#define erase(id, data, ...) \
    erase_(id       , (struct column      *[]){__VA_ARGS__}, \
                 len(((struct column      *[]){__VA_ARGS__})))
