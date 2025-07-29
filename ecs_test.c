#include "ecs.h"
#include "test.h"
#include <stdlib.h>

static void free_component(void *p) {
    component_free(*(void**)p);
}

static sparse_set* meta_of(void *data) {
    return (sparse_set*)data - 1;
}

static void test_attach_detach(void) {
    __attribute__((cleanup(free_component))) int *vals = NULL;

    // Basic ID attach.
    vals = component_attach(vals, sizeof *vals, 1);
    sparse_set *meta = meta_of(vals);
    expect(meta->n   == 1);
    expect(meta->cap == 2);
    expect(meta->ix[1] == 0);
    expect(meta->id[0] == 1);
    vals[meta->ix[1]] = 11;

    // Re-attaching the same ID does nothing.
    int* const prev = vals;
    vals = component_attach(vals, sizeof *vals, 1);
    meta = meta_of(vals);
    expect(vals == prev);
    expect(meta->n == 1);

    // Attach a lower ID, no need to resize ix, but will resize id,vals.
    vals = component_attach(vals, sizeof *vals, 0);
    meta = meta_of(vals);
    expect(vals != prev);
    expect(meta->n   == 2);
    expect(meta->cap == 2);
    expect(meta->ix[0] == 1);
    expect(meta->id[1] == 0);
    vals[meta->ix[0]] = 22;

    // Attach a higher ID, everything grows.
    vals = component_attach(vals, sizeof *vals, 5);
    meta = meta_of(vals);
    expect(meta->n   == 3);
    expect(meta->cap == 6);
    expect(meta->ix[5] == 2);
    expect(meta->id[2] == 5);
    vals[meta->ix[5]] = 55;

    // Detach an unattached ID does nothing.
    component_detach(vals, sizeof *vals, 3);
    meta = meta_of(vals);
    expect(meta->n == 3);

    // Detach an attached ID, swapping the last ID into its place.
    expect(meta->ix[0] == 1);
    component_detach(vals, sizeof *vals, 0);
    meta = meta_of(vals);
    expect(meta->n == 2);
    expect(meta->ix[0] == ~0);
    expect(meta->ix[5] == 1);
    expect(meta->id[1] == 5);
    expect(vals[1] == 55);

    // Keep detatching, another swap.
    component_detach(vals, sizeof *vals, 1);
    meta = meta_of(vals);
    expect(meta->n == 1);
    expect(meta->ix[1] == ~0);
    expect(meta->id[0] == 5);
    expect(meta->ix[5] == 0);
    expect(vals[0] == 55);

    // Detach the last ID.
    component_detach(vals, sizeof *vals, 5);
    meta = meta_of(vals);
    expect(meta->n == 0);
    expect(meta->ix[5] == ~0);
}

static void test_high_id(void) {
    __attribute__((cleanup(free_component))) int *vals = NULL;

    vals = component_attach(vals, sizeof *vals, 7);
    sparse_set *meta = meta_of(vals);
    expect(meta->n   == 1);
    expect(meta->cap == 8);
    expect(meta->ix[7] == 0);
    expect(meta->id[0] == 7);

    component_detach(vals, sizeof *vals, 7);
    meta = meta_of(vals);
    expect(meta->n == 0);
}

static void test_detach_invalid(void) {
    __attribute__((cleanup(free_component))) int *vals = NULL;

    vals = component_attach(vals, sizeof *vals, 1);
    sparse_set *meta = meta_of(vals);
    expect(meta->n == 1);
    component_detach(vals, sizeof *vals, 3);
    meta = meta_of(vals);
    expect(meta->n == 1);

    component_detach(vals, sizeof *vals, 1);
    meta = meta_of(vals);
    expect(meta->n == 0);
    component_detach(vals, sizeof *vals, 1);
    meta = meta_of(vals);
    expect(meta->n == 0);
}

static void test_lookup(void) {
    __attribute__((cleanup(free_component))) int *vals = NULL;

    expect(component_lookup(vals, sizeof *vals, 1) == NULL);

    vals = component_attach(vals, sizeof *vals, 2);
    vals = component_attach(vals, sizeof *vals, 5);
    sparse_set *meta = meta_of(vals);
    vals[meta->ix[2]] = 22;
    vals[meta->ix[5]] = 55;

    expect(component_lookup(vals, sizeof *vals, 2) == vals + meta->ix[2]);
    expect(*(int*)component_lookup(vals, sizeof *vals, 2) == 22);
    expect(component_lookup(vals, sizeof *vals, 5) == vals + meta->ix[5]);
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
