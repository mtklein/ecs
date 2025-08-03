#include "table.h"
#include "test.h"

static void test_basics(void) {
    struct pos {
        float x,y;
    };
    struct stats {
        int hp,ac,atk,def;
    };

    enum {POS, STATS};
    size_t const column[] = {
        [POS]   = sizeof(struct pos),
        [STATS] = sizeof(struct stats),
    };

    struct table t = { .column = column, .columns = sizeof column / sizeof *column };
    int next_id = 0;

    {
        int const id = next_id++;
        struct pos pos = {3,4};
        update(&t,id, &pos, POS,~0);
    }
    {
        int const id = next_id++;
        struct {
            struct pos   pos;
            struct stats stats;
        } cols = {{1,2}, {10,14,2,7}};
        update(&t,id, &cols, POS,STATS,~0);
    }

    struct pos   pos;
    struct stats stats;
    expect( lookup(&t,0,&pos  ,POS  ,~0));
    expect(!lookup(&t,0,&stats,STATS,~0));
    expect( lookup(&t,1,&pos  ,POS  ,~0));
    expect( lookup(&t,1,&stats,STATS,~0));

    int n = 0;
    for (int id = ~0; survey(&t,&id, &pos, POS,~0);) {
        n++;
    }
    expect(n == 2);

    struct {
        struct stats stats;
        struct pos   pos;
    } join;

    n = 0;
    for (int id = ~0; survey(&t,&id, &join, STATS,POS,~0);) {
        expect(id == 1);
        expect(equiv(join.pos.x, 1));
        expect(join.stats.ac == 14);
        n++;
    }
    expect(n == 1);

    {
        struct stats s = {20,30,3,4};
        update(&t,0, &s, STATS,~0);
        expect( lookup(&t,0,&stats, STATS,~0));
        expect(stats.hp == 20);
    }
    {
        struct { struct pos pos; struct stats stats; } patch = {{6,7}, {1,2,3,4}};
        update(&t,1, &patch, POS,STATS,~0);
        struct { struct stats stats; struct pos pos; } got;
        expect( lookup(&t,1,&got, STATS,POS,~0));
        expect(equiv(got.pos.x, 6));
        expect(got.stats.hp == 1);
    }

    n = 0;
    for (int id = ~0; survey(&t,&id, &join, STATS,POS,~0);) {
        n++;
    }
    expect(n == 2);

    table_fini(&t);
}

int main(void) {
    test_basics();
    return 0;
}
