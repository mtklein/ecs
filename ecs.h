#pragma once

#include <stddef.h>


struct branch;

struct component {
    size_t      size;
    struct branch *root;
};

void* component_data(struct component *comp, int entity);
void component_drop(struct component *comp, int entity);
void component_each(struct component *comp, void (*fn)(int entity, void *data, void *ctx), void *ctx);
