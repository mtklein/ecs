#include "ecs.h"
#include "test.h"

static void sum_fn(int entity, void *data, void *ctx) {
    (void)entity;
    int const *val = data;
    int       *sum = ctx;
    *sum += *val;
}

static void count_fn(int entity, void *data, void *ctx) {
    (void)entity;
    (void)data;
    int *count = ctx;
    *count += 1;
}

int main(void) {
    struct component c = {.size = sizeof(int)};

    for (int i = 1; i <= 5; ++i) {
        int *val = component_data(&c, i);
        *val = i * 10;
    }

    {
        int *val = component_data(&c, 3);
        expect(*val == 30);
    }

    {
        int sum = 0;
        component_each(&c, sum_fn, &sum);
        expect(sum == 150);
    }

    component_drop(&c, 3);
    {
        int sum = 0;
        component_each(&c, sum_fn, &sum);
        expect(sum == 120);
    }

    for (int i = 1; i <= 5; ++i) {
        component_drop(&c, i);
    }
    expect(c.root == NULL);

    struct component tag = {.size = 0};

    for (int i = 1; i <= 5; ++i) {
        void *p = component_data(&tag, i);
        expect(p != NULL);
    }

    {
        int count = 0;
        component_each(&tag, count_fn, &count);
        expect(count == 5);
    }

    component_drop(&tag, 3);
    {
        int count = 0;
        component_each(&tag, count_fn, &count);
        expect(count == 4);
    }

    for (int i = 1; i <= 5; ++i) {
        component_drop(&tag, i);
    }
    expect(tag.root == NULL);

    return 0;
}
