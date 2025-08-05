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
        update(id, &((struct pos){3,4}), pos);
    }
    {
        int const id = next_id++;
        struct {
            struct pos   pos;
            struct stats stats;
        } join = {{1,2}, {10,14,2,7}};
        update(id, &join, pos,stats);
    }

    struct pos   p;
    struct stats s;
    expect( lookup(0, &p, pos));
    expect(!lookup(0, &s, stats));
    expect( lookup(1, &p, pos));
    expect( lookup(1, &s, stats));

    int n = 0;
    for (int id = ~0; survey(&id, &p, pos);) {
        n++;
    }
    expect(n == 2);

    struct {
        struct stats stats;
        struct pos   pos;
    } join;

    n = 0;
    for (int id = ~0; survey(&id, &join, stats,pos);) {
        expect(id == 1);
        expect(equiv(join.pos.x, 1));
        expect(join.stats.ac == 14);
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
        update(id, &p, pos);
    }

    struct pos p;
    int n = 0;
    for (int id = ~0; survey(&id, &p, pos); update(id, &p, pos)) {
        p.x += 10;
        p.y += 10;
        n++;
    }
    expect(n == 3);

    for (int id = 0; id < 3; id++) {
        struct pos got;
        expect( lookup(id, &got, pos));
        expect(equiv(got.x, (float)id + 10));
        expect(equiv(got.y, (float)id + 10));
    }
}

int main(void) {
    test_basics();
    test_update_during_iteration();
    return 0;
}
