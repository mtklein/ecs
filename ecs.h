#pragma once

#include <stddef.h>

struct component {
    size_t         size;
    struct branch *root;
};

void* component_data(struct component*, int i);
void  component_drop(struct component*, int i);

void* component_find(struct component const*, int i);
void  component_each(struct component const*, void (*fn)(int i, void *data, void *ctx), void *ctx);
void  component_free(struct component*);
