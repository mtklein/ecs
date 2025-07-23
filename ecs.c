#include "ecs.h"
#include <assert.h>
#include <string.h>

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

void component_set(array *comp, int *ix, void const *val) {
    if (*ix < 0) {
        *ix = push(comp);
    }
    memcpy(ptr(comp, *ix), val, comp->size);
}

void component_del(array *comp, int *ix) {
    (void)comp;
    *ix = ~0;
}

void* component_get(array const *comp, int ix) {
    return ix < 0 ? NULL : ptr(comp, ix);
}
