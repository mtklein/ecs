#include "len.h"
#include "sparse_column.h"
#include "table.h"
#include "test.h"

static void cleanup_column(void *p) {
    struct column *c = *(void**)p;
    c->vptr->drop(c);
}

static void test_basics(void) {
    struct pos {
        float x,y;
    };
    struct stats {
        int hp,ac,atk,def;
    };

    __attribute__((cleanup(cleanup_column)))
    struct column *pos   = sparse_column(sizeof(struct pos)),
                  *stats = sparse_column(sizeof(struct stats));


    int next_id = 0;

    {
        int const id = next_id++;
        update(id, pos,&(struct pos){3,4}, NULL);
    }
    {
        int const id = next_id++;
        struct pos   p = {1,2};
        struct stats s = {10,14,2,7};
        update(id, pos,&p, stats,&s, NULL);
    }

    struct pos   p;
    struct stats s;
    expect( lookup(0, pos,&p,   NULL));
    expect(!lookup(0, stats,&s, NULL));
    expect( lookup(1, pos,&p,   NULL));
    expect( lookup(1, stats,&s, NULL));

    int n = 0;
    for (int id = ~0; survey(&id, pos,&p, NULL);) {
        n++;
    }
    expect(n == 2);

    n = 0;
    for (int id = ~0; survey(&id, stats,&s, pos,&p, NULL);) {
        expect(id == 1);
        expect(equiv(p.x, 1));
        expect(s.ac == 14);
        n++;
    }
    expect(n == 1);

}

static void test_update_during_iteration(void) {
    struct pos {
        float x,y;
    };

    __attribute__((cleanup(cleanup_column)))
    struct column *pos = sparse_column(sizeof(struct pos));

    for (int id = 0; id < 3; id++) {
        struct pos p = { (float)id, (float)id };
        update(id, pos,&p, NULL);
    }

    struct pos p;
    int n = 0;
    for (int id = ~0; survey(&id, pos,&p, NULL); update(id, pos,&p, NULL)) {
        p.x += 10;
        p.y += 10;
        n++;
    }
    expect(n == 3);

    for (int id = 0; id < 3; id++) {
        struct pos got;
        expect( lookup(id, pos,&got, NULL));
        expect(equiv(got.x, (float)id + 10));
        expect(equiv(got.y, (float)id + 10));
    }
}

int main(void) {
    test_basics();
    test_update_during_iteration();
    return 0;
}
