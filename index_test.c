#include "index.h"
#include "test.h"
#include <stdlib.h>

struct point { float x,y; };

int main(void) {
    struct index ix = {.elt = sizeof(struct point)};

    struct point p;
    p = (struct point){47.0f, -47.0f};
    expect(0 == index_insert(&ix, 47, &p));

    p = (struct point){42.0f, -42.0f};
    expect(1 == index_insert(&ix, 42, &p));

    p = (struct point){48.0f, -48.0f};
    expect(2 == index_insert(&ix, 48, &p));

    p = (struct point){50.0f, -50.0f};
    expect(3 == index_insert(&ix, 50, &p));

    struct point* r;
    r = index_lookup(&ix, 47);
    expect(r && (int)r->x == 47 && (int)r->y == -47);
    r = index_lookup(&ix, 42);
    expect(r && (int)r->y == -42);
    r = index_lookup(&ix, 48);
    expect(r && (int)r->y == -48);
    r = index_lookup(&ix, 50);
    expect(r && (int)r->y == -50);
    expect(!index_lookup(&ix, 51));

    r = index_lookup(&ix, 47);
    expect(r && (int)r->y == -47);
    r = index_lookup(&ix, 50);
    expect(r && (int)r->y == -50);

    index_remove(&ix, 42);
    expect(!index_lookup(&ix, 42));
    r = index_lookup(&ix, 50);
    expect(r && (int)r->x == 50);
    r = index_lookup(&ix, 47);
    expect(r && (int)r->x == 47);
    r = index_lookup(&ix, 48);
    expect(r && (int)r->x == 48);

    index_remove(&ix, 42);
    expect(!index_lookup(&ix, 42));

    index_remove(&ix, 48);
    expect(!index_lookup(&ix, 48));
    r = index_lookup(&ix, 47);
    expect(r && (int)r->x == 47);
    r = index_lookup(&ix, 50);
    expect(r && (int)r->x == 50);

    free(ix.sparse);
    free(ix.dense);
    free(ix.data);
    return 0;
}
