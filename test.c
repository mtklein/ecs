#include "ecs.h"
#include <stdio.h>
#include <stdlib.h>

#define expect(x) if (!(x)) fprintf(stderr, "%s:%d expect(%s)\n", __FILE__, __LINE__, #x), \
                            __builtin_debugtrap()

struct point { float x,y; };

int main(void) {
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

    table_drop(&t, 42);
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

    table_drop(&t, 42);
    expect(!table_get(&t, 42));

    table_drop(&t, 48);
    expect(!table_get(&t, 48));

    expect(!table_get(&t, 10000));
    table_drop(&t, 10000);
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

    table_clear(&t);
    return 0;
}
