#include "ecs.h"
#include <stdio.h>
#include <stdlib.h>

static inline void expect_(_Bool x, const char *expr, const char *file, int line) {
    if (!x) { fprintf(stderr, "%s:%d expect(%s)\n", file, line, expr);  __builtin_debugtrap(); }
}
#define expect(x) expect_(x, #x, __FILE__, __LINE__)

static void test_points(void) {
    struct point { float x,y; };
    struct component points = {.size = sizeof(struct point)};

    attach(47, &points, &(struct point){47.0f, -47.f});
    attach(42, &points, &(struct point){42.0f, -42.f});
    attach(48, &points, &(struct point){48.0f, -48.f});
    attach(50, &points, &(struct point){50.0f, -50.f});

    {
        struct point *p = lookup(47, &points);
        expect((int)p->x == 47 && (int)p->y == -47);
    }
    {
        struct point *p = lookup(42, &points);
        expect((int)p->x == 42 && (int)p->y == -42);
    }
    {
        struct point *p = lookup(48, &points);
        expect((int)p->x == 48 && (int)p->y == -48);
    }
    {
        struct point *p = lookup(50, &points);
        expect((int)p->x == 50 && (int)p->y == -50);
    }
    expect(!lookup(51, &points));

    detach(42, &points);
    expect(!lookup(42, &points));
    {
        struct point *p = lookup(47, &points);
        expect((int)p->x == 47);
    }
    {
        struct point *p = lookup(48, &points);
        expect((int)p->x == 48);
    }
    {
        struct point *p = lookup(50, &points);
        expect((int)p->x == 50);
    }

    detach(42, &points);
    expect(!lookup(42, &points));

    detach(48, &points);
    expect(!lookup(48, &points));

    expect(!lookup(10000, &points));
    detach(10000, &points);
    expect(!lookup(10000, &points));

    {
        struct point *p = lookup(47, &points);
        expect((int)p->x == 47);
    }
    {
        struct point *p = lookup(50, &points);
        expect((int)p->x == 50);
    }

    for (struct point *p = points.data, *end = p + points.n; p != end; p++) {
        printf("{%g, %g}\n", (double)p->x, (double)p->y);
    }

    reset(&points);
}

static void test_tag(void) {
    struct component tag = {0};

    expect(!lookup(42, &tag));
    expect(!lookup(47, &tag));

    attach(47, &tag, NULL);
    attach(42, &tag, NULL);

    expect( lookup(42, &tag));
    expect( lookup(47, &tag));

    detach(47, &tag);
    expect( lookup(42, &tag));
    expect(!lookup(47, &tag));

    reset(&tag);
}

static void test_overwrite(void) {
    struct component c = {.size=sizeof(int)};
    int val = 1;
    attach(42, &c, &val);
    val = 2;
    attach(42, &c, &val);
    expect(c.n == 1);
    int const *v = c.data;
    expect(*v == 2);
    reset(&c);

    struct component tag = {0};
    attach(42, &tag, NULL);
    attach(42, &tag, NULL);
    expect(tag.n == 1);
    expect(lookup(42, &tag));
    reset(&tag);
}

int main(void) {
    test_points();
    test_tag();
    test_overwrite();
    return 0;
}

__attribute__((constructor))
static void premain(void) {
    setenv("LLVM_PROFILE_FILE", "%t/tmp.profraw", 0);
}
