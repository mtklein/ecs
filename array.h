#pragma once

#include <stddef.h>

typedef struct {
    size_t const size;
    void        *data;
    int          n,cap;
} array;

int  push(array*);
void* pop(array*);

void* ptr(array const*, int ix);
