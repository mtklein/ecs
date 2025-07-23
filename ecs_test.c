#include "ecs.h"
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

struct ent {
    int ix;
};

int main(void) {
    array entity   = {.size = sizeof(struct ent)};
    array freelist = {.size = sizeof(int)};

    int const id0 = alloc_id(&entity,&freelist);
    assert(id0 == 0);
    assert(entity.n == 1);
    uint8_t *mem = entity.data;
    for (size_t i = 0; i < sizeof(struct ent); i++) {
        assert(mem[i] == 0xff);
    }

    int const id1 = alloc_id(&entity,&freelist);
    assert(id1 == 1);
    assert(entity.n == 2);

    drop_id(&entity,&freelist,id0);
    mem = (uint8_t *)ptr(&entity,id0);
    for (size_t i = 0; i < sizeof(struct ent); i++) {
        assert(mem[i] == 0xff);
    }
    assert(freelist.n == 1);
    assert(*(int *)ptr(&freelist,0) == id0);

    int const id2 = alloc_id(&entity,&freelist);
    assert(id2 == id0);
    assert(freelist.n == 0);

    array comp = {.size = sizeof(int)};
    int ix = -1;
    int v = 1;
    component_set(&comp,&ix,&v);
    assert(ix == 0);
    assert(comp.n == 1);
    assert(*(int *)component_get(&comp,ix) == 1);

    v = 2;
    component_set(&comp,&ix,&v);
    assert(ix == 0);
    assert(comp.n == 1);
    assert(*(int *)component_get(&comp,ix) == 2);

    component_del(&comp,&ix);
    assert(ix == ~0);
    assert(component_get(&comp,ix) == NULL);

    v = 3;
    component_set(&comp,&ix,&v);
    assert(ix == 1);
    assert(comp.n == 2);
    assert(*(int *)component_get(&comp,ix) == 3);

    free(entity.data);
    free(freelist.data);
    free(comp.data);
    return 0;
}
