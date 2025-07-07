#include "ecs.h"
#include <stdio.h>
#include <stdlib.h>

#define len(x) (int)(sizeof x / sizeof *x)

static inline void expect_(_Bool x, const char *expr, const char *file, int line) {
    if (!x) { fprintf(stderr, "%s:%d expect(%s)\n", file, line, expr);  __builtin_debugtrap(); }
}
#define expect(x) expect_(x, #x, __FILE__, __LINE__)

static void test_point_table(void) {
    struct point { float x,y; };
    struct table t = {.size = sizeof(struct point)};

    table_set(&t, 47, &(struct point){47.0f, -47.f});
    table_set(&t, 42, &(struct point){42.0f, -42.f});
    table_set(&t, 48, &(struct point){48.0f, -48.f});
    table_set(&t, 50, &(struct point){50.0f, -50.f});

    {
        struct point *p = table_get(&t, 47);
        expect((int)p->x == 47 && (int)p->y == -47);
    }
    {
        struct point *p = table_get(&t, 42);
        expect((int)p->x == 42 && (int)p->y == -42);
    }
    {
        struct point *p = table_get(&t, 48);
        expect((int)p->x == 48 && (int)p->y == -48);
    }
    {
        struct point *p = table_get(&t, 50);
        expect((int)p->x == 50 && (int)p->y == -50);
    }
    expect(!table_get(&t, 51));

    table_del(&t, 42);
    expect(!table_get(&t, 42));
    {
        struct point *p = table_get(&t, 47);
        expect((int)p->x == 47);
    }
    {
        struct point *p = table_get(&t, 48);
        expect((int)p->x == 48);
    }
    {
        struct point *p = table_get(&t, 50);
        expect((int)p->x == 50);
    }

    table_del(&t, 42);
    expect(!table_get(&t, 42));

    table_del(&t, 48);
    expect(!table_get(&t, 48));

    expect(!table_get(&t, 10000));
    table_del(&t, 10000);
    expect(!table_get(&t, 10000));

    {
        struct point *p = table_get(&t, 47);
        expect((int)p->x == 47);
    }
    {
        struct point *p = table_get(&t, 50);
        expect((int)p->x == 50);
    }

    for (struct point *p = t.data, *end = p + t.n; p != end; p++) {
        printf("{%g, %g}\n", (double)p->x, (double)p->y);
    }

    table_reset(&t);
}

static void test_tag_table(void) {
    struct table tag = {0};

    expect(!table_get(&tag, 42));
    expect(!table_get(&tag, 47));

    table_set(&tag, 47, NULL);
    table_set(&tag, 42, NULL);

    expect( table_get(&tag, 42));
    expect( table_get(&tag, 47));

    table_del(&tag, 47);
    expect( table_get(&tag, 42));
    expect(!table_get(&tag, 47));

    table_reset(&tag);
}

static void test_join(void) {
    struct table ints   = {.size=sizeof(int)};
    struct table floats = {.size=sizeof(float)};
    struct table tag    = {.size=0};

    for (int i = 0; i < 10; i++) {
        float f = 2*(float)i;
        table_set(&ints,     i, &i);
        table_set(&floats, i*2, &f);
        table_set(&tag,    i*4, NULL);
    }

    struct table const *table[] = {&floats,&ints,&tag};

    int key = ~0;
    struct { float f; int i; } vals;

    expect( table_join(table,len(table), &key, &vals)
            && key == 0 && vals.f == 0.0f && vals.i == 0);
    expect( table_join(table,len(table), &key, &vals)
            && key == 4 && vals.f == 4.0f && vals.i == 4);
    expect( table_join(table,len(table), &key, &vals)
            && key == 8 && vals.f == 8.0f && vals.i == 8);
    expect(!table_join(table,len(table), &key, &vals));

    table_reset(&ints);
    table_reset(&floats);
    table_reset(&tag);
}

static void test_join_single(void) {
    struct table ints = {.size = sizeof(int)};
    for (int i = 0; i < 3; i++) {
        table_set(&ints, i, &i);
    }

    struct table const *table[] = {&ints};

    int key = ~0;
    int val = 0;

    expect(table_join(table, len(table), &key, &val) && key == 0 && val == 0);
    expect(table_join(table, len(table), &key, &val) && key == 1 && val == 1);
    expect(table_join(table, len(table), &key, &val) && key == 2 && val == 2);
    expect(!table_join(table, len(table), &key, &val));

    table_reset(&ints);
}

static void test_join_empty(void) {
    struct table t = {.size = sizeof(int)};
    struct table const *table[] = {&t};
    int key = ~0;
    int val = 0;
    expect(!table_join(table, len(table), &key, &val));
    table_reset(&t);
}

static void test_join_duplicate(void) {
    struct table ints = {.size = sizeof(int)};
    struct table floats = {.size = sizeof(float)};
    int i = 0;
    float f = 0.0f;
    table_set(&ints, 0, &i);
    table_set(&ints, 0, &(int){42});
    table_set(&floats, 0, &f);

    struct table const *table[] = {&ints, &floats};

    int key = ~0;
    struct { int i; float f; } vals;

    expect(table_join(table, len(table), &key, &vals)
           && key == 0 && vals.i == 42 && vals.f == 0.0f);
    expect(!table_join(table, len(table), &key, &vals));

    table_reset(&ints);
    table_reset(&floats);
}

int main(void) {
    test_point_table();
    test_tag_table();
    test_join();
    test_join_single();
    test_join_empty();
    test_join_duplicate();
    return 0;
}
