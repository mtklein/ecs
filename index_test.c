#include "index.h"
#include "test.h"
#include <stdlib.h>

int main(void) {
    struct index index = {0};

    expect(0 == index_insert(&index, 47));
    expect(1 == index_insert(&index, 42));
    expect(2 == index_insert(&index, 48));
    expect(3 == index_insert(&index, 50));

    expect(0 == index_lookup(&index, 47));
    expect(1 == index_lookup(&index, 42));
    expect(2 == index_lookup(&index, 48));
    expect(3 == index_lookup(&index, 50));

    expect(~0 == index_lookup(&index, 23));
    expect(~0 == index_lookup(&index, 51));

    index_remove(&index, 42);
    expect(~0 == index_lookup(&index, 42));
    expect(1 == index_lookup(&index, 50));
    expect(0 == index_lookup(&index, 47));
    expect(2 == index_lookup(&index, 48));

    index_remove(&index, 42);
    expect(~0 == index_lookup(&index, 42));

    index_remove(&index, 48);
    expect(~0 == index_lookup(&index, 48));
    expect(0 == index_lookup(&index, 47));
    expect(1 == index_lookup(&index, 50));

    free(index.sparse);
    free(index.dense);
    return 0;
}
