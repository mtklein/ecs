#pragma once

#include <stddef.h>

struct component {
    size_t         size;
    struct branch *root;
};

void* component_data(struct component*, int i);
void  component_drop(struct component*, int i);
void  component_each(struct component*, void (*fn)(int i, void *data, void *ctx), void *ctx);
