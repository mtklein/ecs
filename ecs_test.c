#include "ecs.h"
#include "test.h"
#include <stdlib.h>

static void free_component(void *p) {
    component_free(*(void**)p);
}

static void test_attach_detach(void) {
    __attribute__((cleanup(free_component))) int *vals = NULL;

    // Basic ID attach.
    vals = component_attach(vals, sizeof *vals, 1);
    expect(component_n(vals)   == 1);
    expect(component_ix(vals,1) == 0);
    expect(component_id(vals,0) == 1);
    vals[component_ix(vals,1)] = 11;

    // Re-attaching the same ID does nothing.
    int* const prev = vals;
    vals = component_attach(vals, sizeof *vals, 1);
    expect(vals == prev);
    expect(component_n(vals) == 1);

    // Attach a lower ID, no need to resize ix, but will resize id,vals.
    vals = component_attach(vals, sizeof *vals, 0);
    expect(vals != prev);
    expect(component_n(vals)   == 2);
    expect(component_ix(vals,0) == 1);
    expect(component_id(vals,1) == 0);
    vals[component_ix(vals,0)] = 22;

    // Attach a higher ID, everything grows.
    vals = component_attach(vals, sizeof *vals, 5);
    expect(component_n(vals)   == 3);
    expect(component_ix(vals,5) == 2);
    expect(component_id(vals,2) == 5);
    vals[component_ix(vals,5)] = 55;

    // Detach an unattached ID does nothing.
    component_detach(vals, sizeof *vals, 3);
    expect(component_n(vals) == 3);

    // Detach an attached ID, swapping the last ID into its place.
    expect(component_ix(vals,0) == 1);
    component_detach(vals, sizeof *vals, 0);
    expect(component_n(vals) == 2);
    expect(component_ix(vals,0) == ~0);
    expect(component_ix(vals,5) == 1);
    expect(component_id(vals,1) == 5);
    expect(vals[1] == 55);

    // Keep detatching, another swap.
    component_detach(vals, sizeof *vals, 1);
    expect(component_n(vals) == 1);
    expect(component_ix(vals,1) == ~0);
    expect(component_id(vals,0) == 5);
    expect(component_ix(vals,5) == 0);
    expect(vals[0] == 55);

    // Detach the last ID.
    component_detach(vals, sizeof *vals, 5);
    expect(component_n(vals) == 0);
    expect(component_ix(vals,5) == ~0);
}

static void test_high_id(void) {
    __attribute__((cleanup(free_component))) int *vals = NULL;

    vals = component_attach(vals, sizeof *vals, 7);
    expect(component_n(vals)   == 1);
    expect(component_ix(vals,7) == 0);
    expect(component_id(vals,0) == 7);

    component_detach(vals, sizeof *vals, 7);
    expect(component_n(vals) == 0);
}

static void test_detach_invalid(void) {
    __attribute__((cleanup(free_component))) int *vals = NULL;

    vals = component_attach(vals, sizeof *vals, 1);
    expect(component_n(vals) == 1);
    component_detach(vals, sizeof *vals, 3);
    expect(component_n(vals) == 1);

    component_detach(vals, sizeof *vals, 1);
    expect(component_n(vals) == 0);
    component_detach(vals, sizeof *vals, 1);
    expect(component_n(vals) == 0);
}

static void test_lookup(void) {
    __attribute__((cleanup(free_component))) int *vals = NULL;

    expect(component_lookup(vals, sizeof *vals, 1) == NULL);

    vals = component_attach(vals, sizeof *vals, 2);
    vals = component_attach(vals, sizeof *vals, 5);
    vals[component_ix(vals,2)] = 22;
    vals[component_ix(vals,5)] = 55;

    expect(component_lookup(vals, sizeof *vals, 2) == vals + component_ix(vals,2));
    expect(*(int*)component_lookup(vals, sizeof *vals, 2) == 22);
    expect(component_lookup(vals, sizeof *vals, 5) == vals + component_ix(vals,5));
    expect(*(int*)component_lookup(vals, sizeof *vals, 5) == 55);

    expect(component_lookup(vals, sizeof *vals, 3) == NULL);

    component_detach(vals, sizeof *vals, 2);
    expect(component_lookup(vals, sizeof *vals, 2) == NULL);
    expect(*(int*)component_lookup(vals, sizeof *vals, 5) == 55);

    component_detach(vals, sizeof *vals, 5);
    expect(component_lookup(vals, sizeof *vals, 5) == NULL);
}

int main(void) {
    test_attach_detach();
    test_high_id();
    test_detach_invalid();
    test_lookup();
    return 0;
}
