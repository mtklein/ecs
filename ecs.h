#pragma once

#include <stddef.h>

struct component {
    size_t         size;
    struct branch *root;
};

void* component_data(struct component*, int entity);
void  component_drop(struct component*, int entity);
void  component_each(struct component*, void (*fn)(int entity, void *data, void *ctx), void *ctx);
