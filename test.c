#include "ecs.h"
#include <stdio.h>
#include <stdlib.h>

#define expect(x) if (!(x)) fprintf(stderr, "%s:%d expect(%s)\n", __FILE__, __LINE__, #x), \
                            __builtin_debugtrap()

struct point { float x,y; };

int main(void) {
    struct component c = {.size = sizeof(struct point)};

    component_attach(&c, 47, &(struct point){47.0f, -47.f});
    component_attach(&c, 42, &(struct point){42.0f, -42.f});
    component_attach(&c, 48, &(struct point){48.0f, -48.f});
    component_attach(&c, 50, &(struct point){50.0f, -50.f});

    {
        struct point *p = component_lookup(&c, 47);
        expect((int)p->x == 47 && (int)p->y == -47);
    }
    {
        struct point *p = component_lookup(&c, 42);
        expect((int)p->x == 42 && (int)p->y == -42);
    }
    {
        struct point *p = component_lookup(&c, 48);
        expect((int)p->x == 48 && (int)p->y == -48);
    }
    {
        struct point *p = component_lookup(&c, 50);
        expect((int)p->x == 50 && (int)p->y == -50);
    }
    expect(!component_lookup(&c, 51));

    component_detach(&c, 42);
    expect(!component_lookup(&c, 42));
    {
        struct point *p = component_lookup(&c, 47);
        expect((int)p->x == 47);
    }
    {
        struct point *p = component_lookup(&c, 48);
        expect((int)p->x == 48);
    }
    {
        struct point *p = component_lookup(&c, 50);
        expect((int)p->x == 50);
    }

    component_detach(&c, 42);
    expect(!component_lookup(&c, 42));

    component_detach(&c, 48);
    expect(!component_lookup(&c, 48));

    expect(!component_lookup(&c, 10000));
    component_detach(&c, 10000);
    expect(!component_lookup(&c, 10000));

    {
        struct point *p = component_lookup(&c, 47);
        expect((int)p->x == 47);
    }
    {
        struct point *p = component_lookup(&c, 50);
        expect((int)p->x == 50);
    }

    for (struct point *p = c.data, *end = p + c.n; p != end; p++) {
        printf("{%g, %g}\n", (double)p->x, (double)p->y);
    }

    component_free(&c);
    return 0;
}
