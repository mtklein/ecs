#include "tree.h"
#include "test.h"

static int val(int x) { return x; }

test(basic) {
    __attribute__((cleanup(tree_reset)))
    struct tree t = {.size = sizeof(int)};

    tree_attach(47,&t,&(int){val(47)});
    tree_attach(42,&t,&(int){val(42)});
    tree_attach(48,&t,&(int){val(48)});
    tree_attach(50,&t,&(int){val(50)});

    expect(*(int*)tree_lookup(47,&t) == 47);
    expect(*(int*)tree_lookup(42,&t) == 42);
    expect(*(int*)tree_lookup(48,&t) == 48);
    expect(*(int*)tree_lookup(50,&t) == 50);
    expect(!tree_lookup(51,&t));

    tree_detach(42,&t);
    expect(!tree_lookup(42,&t));
    expect(*(int*)tree_lookup(47,&t) == 47);
    expect(*(int*)tree_lookup(48,&t) == 48);
    expect(*(int*)tree_lookup(50,&t) == 50);

    tree_detach(48,&t);
    expect(!tree_lookup(48,&t));

    expect(*(int*)tree_lookup(47,&t) == 47);
    expect(*(int*)tree_lookup(50,&t) == 50);
}

test(overwrite) {
    __attribute__((cleanup(tree_reset)))
    struct tree t = {.size=sizeof(int)};

    tree_attach(42,&t,&(int){1});
    tree_attach(42,&t,&(int){2});
    expect(t.n == 1);
    expect(*(int*)tree_lookup(42,&t) == 2);
}
