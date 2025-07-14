#include "pma.h"
#include "test.h"

struct point { float x,y; };

static void check_points(struct pma *p) {
    {
        struct point *pt = pma_lookup(47,p);
        expect((int)pt->x == 47 && (int)pt->y == -47);
    }
    {
        struct point *pt = pma_lookup(42,p);
        expect((int)pt->x == 42 && (int)pt->y == -42);
    }
    {
        struct point *pt = pma_lookup(48,p);
        expect((int)pt->x == 48 && (int)pt->y == -48);
    }
    {
        struct point *pt = pma_lookup(50,p);
        expect((int)pt->x == 50 && (int)pt->y == -50);
    }
}

test(pma_points) {
    __attribute__((cleanup(pma_reset)))
    struct pma points = {.size = sizeof(struct point)};
    pma_attach(47,&points,&(struct point){47.f,-47.f});
    pma_attach(42,&points,&(struct point){42.f,-42.f});
    pma_attach(48,&points,&(struct point){48.f,-48.f});
    pma_attach(50,&points,&(struct point){50.f,-50.f});
    check_points(&points);
    expect(!pma_lookup(51,&points));
    pma_detach(42,&points);
    expect(!pma_lookup(42,&points));
    pma_detach(42,&points);
    pma_detach(48,&points);
    expect(!pma_lookup(48,&points));
    expect(!pma_lookup(10000,&points));
    pma_detach(10000,&points);
    expect(!pma_lookup(10000,&points));
}

test(pma_tag) {
    __attribute__((cleanup(pma_reset)))
    struct pma tag = {0};
    expect(!pma_lookup(42,&tag));
    expect(!pma_lookup(47,&tag));
    pma_attach(47,&tag,NULL);
    pma_attach(42,&tag,NULL);
    expect(pma_lookup(42,&tag));
    expect(pma_lookup(47,&tag));
    pma_detach(47,&tag);
    expect(pma_lookup(42,&tag));
    expect(!pma_lookup(47,&tag));
}

test(pma_overwrite) {
    __attribute__((cleanup(pma_reset)))
    struct pma p = {.size=sizeof(int)};
    int v=1;
    pma_attach(42,&p,&v);
    v=2;
    pma_attach(42,&p,&v);
    expect(p.n==1);
    int const *val = p.data;
    expect(*val==2);
    __attribute__((cleanup(pma_reset)))
    struct pma tag={0};
    pma_attach(42,&tag,NULL);
    pma_attach(42,&tag,NULL);
    expect(tag.n==1);
    expect(pma_lookup(42,&tag));
}
