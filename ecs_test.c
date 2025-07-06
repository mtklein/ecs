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

static void test_int_component(void) {
    struct component c = {.size = sizeof(int)};

    for (int i = 1; i <= 5; ++i) {
        int *val = component_data(&c, i);
        *val = i * 10;
    }

    expect(component_find(&c, 0) == NULL);
    for (int i = 1; i <= 5; ++i) {
        int *val = component_find(&c, i);
        expect(val != NULL);
        expect(*val == i * 10);
    }
    expect(component_find(&c, 6) == NULL);

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
    expect(component_find(&c, 3) == NULL);
    {
        int sum = 0;
        component_each(&c, sum_fn, &sum);
        expect(sum == 120);
    }

    for (int i = 1; i <= 5; ++i) {
        component_drop(&c, i);
    }
    expect(c.root == NULL);
}

static void test_tag_component(void) {
    struct component tag = {.size = 0};

    for (int i = 1; i <= 5; ++i) {
        void *p = component_data(&tag, i);
        expect(p != NULL);
    }

    expect(component_find(&tag, 0) == NULL);
    for (int i = 1; i <= 5; ++i) {
        expect(component_find(&tag, i) != NULL);
    }
    expect(component_find(&tag, 6) == NULL);

    {
        int count = 0;
        component_each(&tag, count_fn, &count);
        expect(count == 5);
    }

    component_drop(&tag, 3);
    expect(component_find(&tag, 3) == NULL);
    {
        int count = 0;
        component_each(&tag, count_fn, &count);
        expect(count == 4);
    }

    for (int i = 1; i <= 5; ++i) {
        component_drop(&tag, i);
    }
    expect(tag.root == NULL);
    expect(component_find(&tag, 1) == NULL);
}

static void test_drop_end(void) {
    struct component drop_end = {.size = sizeof(int)};
    for (int i = 1; i <= 3; ++i) {
        int *val = component_data(&drop_end, i);
        *val = i;
    }
    component_drop(&drop_end, 3);
    expect(component_find(&drop_end, 3) == NULL);
    expect(component_find(&drop_end, 2) != NULL);
    for (int i = 1; i <= 2; ++i) {
        component_drop(&drop_end, i);
    }

}

static void test_merge(void) {
    struct component merge = {.size = sizeof(int)};
    int *v1 = component_data(&merge, 1);
    *v1 = 1;
    int *v2 = component_data(&merge, 2);
    *v2 = 2;
    int *v4 = component_data(&merge, 4);
    *v4 = 4;
    int *v5 = component_data(&merge, 5);
    *v5 = 5;
    int *v3 = component_data(&merge, 3);
    *v3 = 3;
    int sum = 0;
    component_each(&merge, sum_fn, &sum);
    expect(sum == 15);
    for (int i = 1; i <= 5; ++i) {
        component_drop(&merge, i);
    }
}

static void test_rotate_left(void) {
    struct component rot_l = {.size = 0};
    component_data(&rot_l, 30);
    component_data(&rot_l, 20);
    component_data(&rot_l, 10);
    expect(component_find(&rot_l, 10) != NULL);
    expect(component_find(&rot_l, 20) != NULL);
    expect(component_find(&rot_l, 30) != NULL);
    component_drop(&rot_l, 10);
    component_drop(&rot_l, 20);
    component_drop(&rot_l, 30);
}

static void test_rotate_right(void) {
    struct component rot_r = {.size = 0};
    component_data(&rot_r, 10);
    component_data(&rot_r, 20);
    component_data(&rot_r, 30);
    expect(component_find(&rot_r, 10) != NULL);
    expect(component_find(&rot_r, 20) != NULL);
    expect(component_find(&rot_r, 30) != NULL);
    component_drop(&rot_r, 10);
    component_drop(&rot_r, 20);
    component_drop(&rot_r, 30);
}

static void test_double_rotate_lr(void) {
    struct component t = {.size = 0};
    component_data(&t, 30);
    component_data(&t, 10);
    component_data(&t, 20);
    expect(component_find(&t, 10) != NULL);
    expect(component_find(&t, 20) != NULL);
    expect(component_find(&t, 30) != NULL);
    component_drop(&t, 10);
    component_drop(&t, 20);
    component_drop(&t, 30);
}

static void test_double_rotate_rl(void) {
    struct component t = {.size = 0};
    component_data(&t, 10);
    component_data(&t, 30);
    component_data(&t, 20);
    expect(component_find(&t, 10) != NULL);
    expect(component_find(&t, 20) != NULL);
    expect(component_find(&t, 30) != NULL);
    component_drop(&t, 10);
    component_drop(&t, 20);
    component_drop(&t, 30);
}

static void test_remove_min(void) {
    struct component c = {.size = sizeof(int)};
    int *v20 = component_data(&c, 20);
    *v20 = 20;
    int *v10 = component_data(&c, 10);
    *v10 = 10;
    int *v30 = component_data(&c, 30);
    *v30 = 30;
    int *v25 = component_data(&c, 25);
    *v25 = 25;
    component_drop(&c, 20);
    expect(component_find(&c, 20) == NULL);
    expect(component_find(&c, 10) != NULL);
    expect(component_find(&c, 25) != NULL);
    expect(component_find(&c, 30) != NULL);
    component_drop(&c, 10);
    component_drop(&c, 25);
    component_drop(&c, 30);
}

static void test_succ_no_pred(void) {
    struct component c = {.size = sizeof(int)};
    int *v3 = component_data(&c, 3);
    *v3 = 3;
    int *v2 = component_data(&c, 2);
    *v2 = 2;
    expect(component_find(&c, 2) != NULL);
    expect(component_find(&c, 3) != NULL);
    component_drop(&c, 2);
    component_drop(&c, 3);
    expect(c.root == NULL);
}

static void test_succ_with_pred(void) {
    struct component c = {.size = sizeof(int)};
    int *v1 = component_data(&c, 1);
    *v1 = 1;
    int *v3 = component_data(&c, 3);
    *v3 = 3;
    int *v2 = component_data(&c, 2);
    *v2 = 2;
    expect(component_find(&c, 1) != NULL);
    expect(component_find(&c, 2) != NULL);
    expect(component_find(&c, 3) != NULL);
    component_drop(&c, 1);
    component_drop(&c, 2);
    component_drop(&c, 3);
    expect(c.root == NULL);
}

int main(void) {
    test_int_component();
    test_tag_component();
    test_drop_end();
    test_merge();
    test_rotate_left();
    test_rotate_right();
    test_double_rotate_lr();
    test_double_rotate_rl();
    test_remove_min();
    test_succ_no_pred();
    test_succ_with_pred();
    return 0;
}
