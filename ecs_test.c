#include "ecs.h"
#include "test.h"

static void sum_fn(int entity, void *data, void *ctx) {
    (void)entity;
    *(int *)ctx += *(int *)data;
}

int main(void) {
    struct component c = { .size = sizeof(int), .root = NULL };

    for (int i = 1; i <= 5; ++i)
        *(int *)component_data(&c, i) = i * 10;

    expect(*(int *)component_data(&c, 3) == 30);

    int sum = 0;
    component_each(&c, sum_fn, &sum);
    expect(sum == 150);

    component_drop(&c, 3);
    sum = 0;
    component_each(&c, sum_fn, &sum);
    expect(sum == 120);

    for (int i = 1; i <= 5; ++i)
        component_drop(&c, i);
    expect(c.root == NULL);

    return 0;
}
