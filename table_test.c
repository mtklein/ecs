#include "len.h"
#include "sparse_column.h"
#include "sorted_column.h"
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

static int cmp_id(int a_id, void const *a, int b_id, void const *b) {
    (void)a;(void)b;
    return a_id - b_id;
}

static void test_sorted_by_id(void) {
    __attribute__((cleanup(cleanup_column)))
    struct column *c = sorted_column(sizeof(int), cmp_id);

    int v;
    v = 1; update(2, &v, c);
    v = 2; update(0, &v, c);
    v = 3; update(1, &v, c);

    int expect_ids[] = {0,1,2};
    int n = 0;
    for (int id = ~0; survey(&id, &v, c); n++) {
        expect(id == expect_ids[n]);
    }
    expect(n == len(expect_ids));
}

struct scan_pos { int x,y; };

static int cmp_scanline(int a_id, void const *a, int b_id, void const *b) {
    (void)a_id;(void)b_id;
    struct scan_pos const *ap = a;
    struct scan_pos const *bp = b;
    if (ap->y != bp->y) {
        return ap->y - bp->y;
    }
    if (ap->x != bp->x) {
        return ap->x - bp->x;
    }
    return 0;
}

static void test_sorted_scanline(void) {
    __attribute__((cleanup(cleanup_column)))
    struct column *c = sorted_column(sizeof(struct scan_pos), cmp_scanline);

    update(1, &((struct scan_pos){1,0}), c);
    update(2, &((struct scan_pos){0,0}), c);
    update(3, &((struct scan_pos){1,1}), c);
    update(4, &((struct scan_pos){0,1}), c);

    int ids[] = {2,1,4,3};
    struct scan_pos expect_pos[] = {{0,0},{1,0},{0,1},{1,1}};
    struct scan_pos p;
    int n = 0;
    for (int id = ~0; survey(&id, &p, c); n++) {
        expect(id == ids[n]);
        expect(p.x == expect_pos[n].x);
        expect(p.y == expect_pos[n].y);
    }
    expect(n == len(ids));
}

int main(void) {
    test_basics();
    test_update_during_iteration();
    test_sorted_by_id();
    test_sorted_scanline();
    return 0;
}
