#pragma once

#include <stddef.h>

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpadded"
#endif
struct leaf {
    struct leaf *left;
    struct leaf *right;
    int height;
    int begin;
    int end; /* exclusive */
    char data[]; /* `end - begin` slots of component data */
};
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

struct component {
    size_t size;            /* size of the POD stored per entity */
    struct leaf *root;      /* root of the entity tree */
};

void *component_data(struct component *, int entity);
void component_drop(struct component *, int entity);
void component_each(struct component *, void(*fn)(int entity, void *data, void *ctx), void *ctx);
