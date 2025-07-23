#include "ecs.h"
#include "test.h"
#include <stdlib.h>

static void free_data(void *p) {
    array *arr = p;
    free(arr->data);
    arr->data = NULL;
}

static void free_comp(void *p) {
    component *comp = p;
    free(comp->data);
    free(comp->id);
    comp->data = NULL;
    comp->id   = NULL;
}

struct entity {
    int a,b;
};

static void test_ids(void) {
    __attribute__((cleanup(free_data)))
    array entity   = {.size = sizeof(struct entity)},
          freelist = {.size = sizeof(int)};

    int const id = alloc_id(&entity, &freelist);
    {
        expect(      id == 0);
        expect(entity.n == 1);

        struct entity *e = ptr(&entity, id);
        expect(e->a == ~0);
        expect(e->b == ~0);
        e->a = 42;
        e->b = 47;
    }

    int const next = alloc_id(&entity, &freelist);
    {
        expect(    next == 1);
        expect(entity.n == 2);

        struct entity *e = ptr(&entity, next);
        expect(e->a == ~0);
        expect(e->b == ~0);
        e->a = 12;
        e->b = 17;
    }

    drop_id(&entity, &freelist, id);
    {
        expect(entity.n == 2);
        struct entity const *e = entity.data;
        expect(e[  id].a == ~0);
        expect(e[  id].b == ~0);
        expect(e[next].a == 12);
        expect(e[next].b == 17);

        expect(freelist.n == 1);
        int const *free_id = freelist.data;
        expect(*free_id == id);
    }

    int const reuse = alloc_id(&entity, &freelist);
    {
        expect(     reuse == id);
        expect(  entity.n ==  2);
        expect(freelist.n ==  0);

        struct entity const *e = entity.data;
        expect(e[reuse].a == ~0);
        expect(e[reuse].b == ~0);
        expect(e[ next].a == 12);
        expect(e[ next].b == 17);
    }
}

static void test_components(void) {
    __attribute__((cleanup(free_comp)))
    component comp = {.size = sizeof(int)};

    int a = -1, b = -1;
    int val = 1;
    component_set(&comp, &a, 0, &val);

    val = 2;
    component_set(&comp, &b, 1, &val);

    expect(comp.n == 2);
    expect(a == 0);
    expect(b == 1);

    component_del(&comp, &a, &b);
    expect(a == ~0);
    expect(b == 0);
    expect(comp.n == 1);
    expect(*(int*)component_get(&comp, b) == val);

    val = 3;
    component_set(&comp, &a, 0, &val);
    expect(a == 1);
    expect(comp.n == 2);
    expect(*(int*)component_get(&comp, a) == val);

    component_del(&comp, &b, &a);
    expect(b == ~0);
    expect(a == 0);
    expect(comp.n == 1);

    component_del(&comp, &a, &b);
    expect(a == ~0);
    expect(b == ~0);
    expect(comp.n == 0);
}

int main(void) {
    test_ids();
    test_components();
    return 0;
}
