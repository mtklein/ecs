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

struct join_row { int key,a,b,c; };

static void test_table_join(void) {
    struct table a = {.size = sizeof(int)};
    struct table b = {.size = sizeof(int)};
    struct table c = {.size = sizeof(int)};
    table_set(&a, 1, &(int){10});
    table_set(&a, 3, &(int){30});
    table_set(&b, 1, &(int){100});
    table_set(&b, 2, &(int){200});
    table_set(&c, 3, &(int){300});

    struct table const *table[] = {&a,&b,&c};
    struct join_row got[2] = {0};
    int vals[3];
    int ix = 0;
    for (int key = 0; table_join(table, 3, &key, vals);) {
        got[ix].key = key;
        got[ix].a = vals[0];
        got[ix].b = vals[1];
        got[ix].c = vals[2];
        ix++;
    }

    expect(ix == 2);
    expect(got[0].key == 1 && got[0].a == 10 && got[0].b == 100 && got[0].c == 0);
    expect(got[1].key == 3 && got[1].a == 30 && got[1].b == 0 && got[1].c == 300);

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
