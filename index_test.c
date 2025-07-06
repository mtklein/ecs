#include "index.h"
#include "test.h"
#include <stdlib.h>

struct point { float x,y; };

int main(void) {
    struct index ix = {.elt = sizeof(struct point)};

    index_insert(&ix, 47, &(struct point){47.0f, -47.f});
    index_insert(&ix, 42, &(struct point){42.0f, -42.f});
    index_insert(&ix, 48, &(struct point){48.0f, -48.f});
    index_insert(&ix, 50, &(struct point){50.0f, -50.f});

    {
        struct point *p = index_lookup(&ix, 47);
        expect((int)p->x == 47 && (int)p->y == -47);
    }
    {
        struct point *p = index_lookup(&ix, 42);
        expect((int)p->x == 42 && (int)p->y == -42);
    }
    {
        struct point *p = index_lookup(&ix, 48);
        expect((int)p->x == 48 && (int)p->y == -48);
    }
    {
        struct point *p = index_lookup(&ix, 50);
        expect((int)p->x == 50 && (int)p->y == -50);
    }
    expect(!index_lookup(&ix, 51));

    index_remove(&ix, 42);
    expect(!index_lookup(&ix, 42));
    {
        struct point *p = index_lookup(&ix, 47);
        expect((int)p->x == 47);
    }
    {
        struct point *p = index_lookup(&ix, 48);
        expect((int)p->x == 48);
    }
    {
        struct point *p = index_lookup(&ix, 50);
        expect((int)p->x == 50);
    }

    index_remove(&ix, 42);
    expect(!index_lookup(&ix, 42));

    index_remove(&ix, 48);
    expect(!index_lookup(&ix, 48));

    expect(!index_lookup(&ix, 10000));
    index_remove(&ix, 10000);
    expect(!index_lookup(&ix, 10000));

    {
        struct point *p = index_lookup(&ix, 47);
        expect((int)p->x == 47);
    }
    {
        struct point *p = index_lookup(&ix, 50);
        expect((int)p->x == 50);
    }

    for (struct point *p = ix.data, *end = p+ix.vals; p != end; p++) {
        printf("{%g, %g}\n", (double)p->x, (double)p->y);
    }

    free(ix.sparse);
    free(ix.dense);
    free(ix.data);
    return 0;
}
