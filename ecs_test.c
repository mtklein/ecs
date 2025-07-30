#include "ecs.h"
#include "test.h"
#include <stdlib.h>

static void free_component(void *p) {
    component(void) *c = p;
    free(c->data);
    free(c->id);
    free(c->ix);
}

static void test_attach_detach(void) {
    __attribute__((cleanup(free_component))) component(int) comp = {.size=sizeof(int)};

    // Basic ID attach.
    component_attach(&comp, 1);
    expect(comp.n   == 1);
    expect(comp.cap == 2);
    expect(comp.ix[1] == 0);
    expect(comp.id[0] == 1);
    ((int*)comp.data)[comp.ix[1]] = 11;

    // Re-attaching the same ID does nothing.
    void *prev = comp.data;
    component_attach(&comp, 1);
    expect(comp.data == prev);
    expect(comp.n == 1);

    // Attach a lower ID, no need to resize ix, but will resize id,vals.
    component_attach(&comp, 0);
    expect(comp.data != prev);
    expect(comp.n   == 2);
    expect(comp.cap == 2);
    expect(comp.ix[0] == 1);
    expect(comp.id[1] == 0);
    ((int*)comp.data)[comp.ix[0]] = 22;

    // Attach a higher ID, everything grows.
    component_attach(&comp, 5);
    expect(comp.n   == 3);
    expect(comp.cap == 6);
    expect(comp.ix[5] == 2);
    expect(comp.id[2] == 5);
    ((int*)comp.data)[comp.ix[5]] = 55;

    // Detach an unattached ID does nothing.
    component_detach(&comp, 3);
    expect(comp.n == 3);

    // Detach an attached ID, swapping the last ID into its place.
    expect(comp.ix[0] == 1);
    component_detach(&comp, 0);
    expect(comp.n == 2);
    expect(comp.ix[0] == ~0);
    expect(comp.ix[5] == 1);
    expect(comp.id[1] == 5);
    expect(((int*)comp.data)[1] == 55);

    // Keep detatching, another swap.
    component_detach(&comp, 1);
    expect(comp.n == 1);
    expect(comp.ix[1] == ~0);
    expect(comp.id[0] == 5);
    expect(comp.ix[5] == 0);
    expect(((int*)comp.data)[0] == 55);

    // Detach the last ID.
    component_detach(&comp, 5);
    expect(comp.n == 0);
    expect(comp.ix[5] == ~0);
}

static void test_high_id(void) {
    __attribute__((cleanup(free_component))) component(int) comp = {.size=sizeof(int)};

    component_attach(&comp, 7);
    expect(comp.n   == 1);
    expect(comp.cap == 8);
    expect(comp.ix[7] == 0);
    expect(comp.id[0] == 7);

    component_detach(&comp, 7);
    expect(comp.n == 0);
}

static void test_detach_invalid(void) {
    __attribute__((cleanup(free_component))) component(int) comp = {.size=sizeof(int)};

    component_attach(&comp, 1);
    expect(comp.n == 1);
    component_detach(&comp, 3);
    expect(comp.n == 1);

    component_detach(&comp, 1);
    expect(comp.n == 0);
    component_detach(&comp, 1);
    expect(comp.n == 0);
}

static void test_lookup(void) {
    __attribute__((cleanup(free_component))) component(int) comp = {.size=sizeof(int)};

    expect(component_lookup(&comp, 1) == NULL);

    component_attach(&comp, 2);
    component_attach(&comp, 5);
    ((int*)comp.data)[comp.ix[2]] = 22;
    ((int*)comp.data)[comp.ix[5]] = 55;

    expect(component_lookup(&comp, 2) == (char*)comp.data + (size_t)comp.ix[2]*sizeof(int));
    expect(*(int*)component_lookup(&comp, 2) == 22);
    expect(component_lookup(&comp, 5) == (char*)comp.data + (size_t)comp.ix[5]*sizeof(int));
    expect(*(int*)component_lookup(&comp, 5) == 55);

    expect(component_lookup(&comp, 3) == NULL);

    component_detach(&comp, 2);
    expect(component_lookup(&comp, 2) == NULL);
    expect(*(int*)component_lookup(&comp, 5) == 55);

    component_detach(&comp, 5);
    expect(component_lookup(&comp, 5) == NULL);
}

int main(void) {
    test_attach_detach();
    test_high_id();
    test_detach_invalid();
    test_lookup();
    return 0;
}
