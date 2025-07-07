#include "ecs.h"
#include "stdlib.h"
#include <stdio.h>

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

static void collect(int key, void* const row[], void *ctx) {
    struct {
        int key[4];
        int a[4];
        int b[4];
        int c[4];
        int n;
    } *g = ctx;
    g->key[g->n] = key;
    g->a[g->n] = row[0] ? *(int*)row[0] : 0;
    g->b[g->n] = row[1] ? *(int*)row[1] : 0;
    g->c[g->n] = row[2] ? *(int*)row[2] : 0;
    g->n++;
}

static void test_table_join(void) {
    struct table a = {.size = sizeof(int)};
    struct table b = {.size = sizeof(int)};
    struct table c = {.size = sizeof(int)};

    for (int i = 0; i < 5; ++i) { int v = i;       table_set(&a, i, &v); }
    for (int i = 2; i < 5; ++i) { int v = i*10;    table_set(&b, i, &v); }
    for (int i = 3; i < 5; ++i) { int v = i*100;   table_set(&c, i, &v); }

    struct {
        int key[4];
        int a[4];
        int b[4];
        int c[4];
        int n;
    } g = {0};

    struct table const *table[] = {&a,&b,&c};
    table_join(table, 3, &g, collect);

    expect(g.n == 2);
    expect(g.key[0] == 3 && g.a[0] == 3  && g.b[0] == 30  && g.c[0] == 300);
    expect(g.key[1] == 4 && g.a[1] == 4  && g.b[1] == 40  && g.c[1] == 400);

    table_reset(&a);
    table_reset(&b);
    table_reset(&c);
}

int main(void) {
    test_point_table();
    test_tag_table();
    test_table_join();
    return 0;
}
