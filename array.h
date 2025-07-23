#pragma once

#include <stddef.h>

typedef struct {
    size_t const size;
    void        *data;
    int          n,cap;
} array;

int  grow(array*);
void* ptr(array const*, int ix);
void* pop(array*);
