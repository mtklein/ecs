#include "ecs.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>

int alloc_id(array *entity, array *freelist) {
    assert(freelist->size == sizeof(int));

    for (int const *id = pop(freelist); id;) {
        return *id;
    }
    int const id = push(entity);
    memset(ptr(entity, id), ~0, entity->size);
    return id;
}

void drop_id(array *entity, array *freelist, int id) {
    assert(freelist->size == sizeof(int));

    memset(ptr(entity, id), ~0, entity->size);
    int *free_id = ptr(freelist, push(freelist));
    *free_id = id;
}

static int push_component(component *c) {
    if (c->cap == c->n) {
        c->cap  = c->cap ? 2*c->cap : 1;
        c->data = realloc(c->data, (size_t)c->cap * c->size);
        c->id   = realloc(c->id,  (size_t)c->cap * sizeof *c->id);
    }
    return c->n++;
}

static void* ptr_component(component const *c, int ix) {
    return (char*)c->data + (size_t)ix * c->size;
}

void component_set(component *comp, int *ix, int owner, void const *val) {
    if (*ix < 0) {
        *ix = push_component(comp);
        comp->id[*ix] = owner;
    }
    memcpy(ptr_component(comp, *ix), val, comp->size);
}

void component_del(component *comp, int *ix, int *back) {
    if (*ix >= 0) {
        int const last = comp->n - 1;
        if (*ix != last) {
            void *dst = ptr_component(comp, *ix);
            void *src = ptr_component(comp, last);
            memcpy(dst, src, comp->size);
            comp->id[*ix] = comp->id[last];
            *back = *ix;
        }
        comp->n--;
        *ix = ~0;
    }
}

void* component_get(component const *comp, int ix) {
    return ix < 0 ? NULL : ptr_component(comp, ix);
}
