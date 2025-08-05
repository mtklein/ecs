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

#if 0
    {
        struct stats s = {20,30,3,4};
        update(&t,0, &s, STATS);
        expect( lookup(&t,0,&stats, STATS));
        expect(stats.hp == 20);
    }
    {
        struct { struct pos pos; struct stats stats; } patch = {{6,7}, {1,2,3,4}};
        update(&t,1, &patch, POS,STATS);
        struct { struct stats stats; struct pos pos; } got;
        expect( lookup(&t,1,&got, STATS,POS));
        expect(equiv(got.pos.x, 6));
        expect(got.stats.hp == 1);
    }

    n = 0;
    for (int id = ~0; survey(&t,&id, &join, STATS,POS);) {
        n++;
    }
    expect(n == 2);
#endif
}

static void test_update_during_iteration(void) {
#if 0
    struct pos {
        float x,y;
    };

    enum {POS};
    size_t const column_size[] = {
        [POS] = sizeof(struct pos),
    };

    struct table t = { .column_size = column_size, .columns = len(column_size) };

    for (int id = 0; id < 3; id++) {
        struct pos pos = { (float)id, (float)id };
        update(&t,id, &pos, POS);
    }

    struct pos pos;
    int n = 0;
    for (int id = ~0; survey(&t,&id, &pos, POS); update(&t,id, &pos, POS)) {
        pos.x += 10;
        pos.y += 10;
        n++;
    }
    expect(n == 3);

    for (int id = 0; id < 3; id++) {
        struct pos got;
        expect( lookup(&t,id,&got, POS));
        expect(equiv(got.x, (float)id + 10));
        expect(equiv(got.y, (float)id + 10));
    }
#endif
}

int main(void) {
    test_basics();
    test_update_during_iteration();
    return 0;
}
