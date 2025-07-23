#include "ecs.h"
#include "test.h"
#include <stdlib.h>

static void free_ptr(void *p) {
    free(*(void**)p);
}

static void free_comp(void *p) {
    component *comp = p;
    free(comp->id);
    free(comp->ix);
}

static void test_attach_detach(void) {
    __attribute__((cleanup(free_ptr))) int *vals = NULL;
    __attribute__((cleanup(free_comp))) component comp = {0};

    vals = component_attach(vals, sizeof *vals, &comp, 1);
    expect(comp.n == 1);
    expect(comp.cap == 2);
    expect(comp.id[0] == 1);
    expect(comp.ix[1] == 0);
    vals[comp.ix[1]] = 11;

    int *prev = vals;
    vals = component_attach(vals, sizeof *vals, &comp, 1);
    expect(vals == prev);
    expect(comp.n == 1);

    vals = component_attach(vals, sizeof *vals, &comp, 0);
    expect(comp.n == 2);
    expect(comp.id[1] == 0);
    expect(comp.ix[0] == 1);
    vals[comp.ix[0]] = 22;

    vals = component_attach(vals, sizeof *vals, &comp, 5);
    expect(comp.n == 3);
    expect(comp.cap == 6);
    expect(comp.id[2] == 5);
    expect(comp.ix[5] == 2);
    vals[comp.ix[5]] = 55;

    component_detach(vals, sizeof *vals, &comp, 3);
    expect(comp.n == 3);

    component_detach(vals, sizeof *vals, &comp, 0);
    expect(comp.n == 2);
    expect(comp.ix[0] == ~0);
    expect(comp.id[1] == 5);
    expect(comp.ix[5] == 1);
    expect(vals[1] == 55);

    component_detach(vals, sizeof *vals, &comp, 1);
    expect(comp.n == 1);
    expect(comp.ix[1] == ~0);
    expect(comp.id[0] == 5);
    expect(comp.ix[5] == 0);
    expect(vals[0] == 55);

    component_detach(vals, sizeof *vals, &comp, 5);
    expect(comp.n == 0);
    expect(comp.ix[5] == ~0);
}

static void test_high_id(void) {
    __attribute__((cleanup(free_ptr))) int *vals = NULL;
    __attribute__((cleanup(free_comp))) component comp = {0};

    vals = component_attach(vals, sizeof *vals, &comp, 7);
    expect(comp.cap == 8);
    expect(comp.n == 1);
    expect(comp.id[0] == 7);
    expect(comp.ix[7] == 0);

    component_detach(vals, sizeof *vals, &comp, 7);
    expect(comp.n == 0);
}

static void test_detach_invalid(void) {
    __attribute__((cleanup(free_ptr))) int *vals = NULL;
    __attribute__((cleanup(free_comp))) component comp = {0};

    vals = component_attach(vals, sizeof *vals, &comp, 1);
    component_detach(vals, sizeof *vals, &comp, 3);
    expect(comp.n == 1);

    component_detach(vals, sizeof *vals, &comp, 1);
    component_detach(vals, sizeof *vals, &comp, 1);
    expect(comp.n == 0);
}

int main(void) {
    test_attach_detach();
    test_high_id();
    test_detach_invalid();
    return 0;
}


