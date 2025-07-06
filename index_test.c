#include "index.h"
#include "test.h"
#include <stdlib.h>

struct point { float x,y; };

static struct point* val_at(struct index *ix, int idx) {
    struct point *p = ix->data;
    return p + idx;
}

int main(void) {
    struct index ix = {.elt = sizeof(struct point)};

    expect(0 == index_insert(&ix, 47));
    *val_at(&ix, 0) = (struct point){47.0f, -47.0f};

    expect(1 == index_insert(&ix, 42));
    *val_at(&ix, 1) = (struct point){42.0f, -42.0f};

    expect(2 == index_insert(&ix, 48));
    *val_at(&ix, 2) = (struct point){48.0f, -48.0f};

    expect(3 == index_insert(&ix, 50));
    *val_at(&ix, 3) = (struct point){50.0f, -50.0f};

    expect( 0 == index_lookup(&ix, 47));
    expect( 1 == index_lookup(&ix, 42));
    expect( 2 == index_lookup(&ix, 48));
    expect( 3 == index_lookup(&ix, 50));
    expect(~0 == index_lookup(&ix, 51));

    expect((int)val_at(&ix, index_lookup(&ix, 47))->y == -47);
    expect((int)val_at(&ix, index_lookup(&ix, 50))->y == -50);

    index_remove(&ix, 42);
    expect(~0 == index_lookup(&ix, 42));
    expect( 1 == index_lookup(&ix, 50));
    expect((int)val_at(&ix, 1)->x == 50);
    expect( 0 == index_lookup(&ix, 47));
    expect((int)val_at(&ix, 0)->x == 47);
    expect( 2 == index_lookup(&ix, 48));

    index_remove(&ix, 42);
    expect(~0 == index_lookup(&ix, 42));

    index_remove(&ix, 48);
    expect(~0 == index_lookup(&ix, 48));
    expect( 0 == index_lookup(&ix, 47));
    expect( (int)val_at(&ix, 0)->x == 47);
    expect( 1 == index_lookup(&ix, 50));
    expect( (int)val_at(&ix, 1)->x == 50);

    free(ix.sparse);
    free(ix.dense);
    free(ix.data);
    return 0;
}
