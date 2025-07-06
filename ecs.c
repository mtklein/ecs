#include "ecs.h"
#include <stdlib.h>
#include <string.h>

struct branch {
    struct branch *L,*R;
    int  height;
    int  begin,end;
    int  cap;
    char data[];
};

static int height(struct branch *b) {
    return b ? b->height : 0;
}

static int max(int x, int y) {
    return x>y ? x : y;
}

static int next_pow2(int n) {
    int cap = 1;
    while (cap < n) {
        cap *= 2;
    }
    return cap;
}

static void recalculate_height(struct branch *b) {
    b->height = 1 + max(height(b->L),
                        height(b->R));
}

static struct branch* rotate_right(struct branch *y) {
    struct branch *x = y->L;
    y->L = x->R;
    x->R = y;
    recalculate_height(y);
    recalculate_height(x);
    return x;
}

static struct branch* rotate_left(struct branch *x) {
    struct branch *y = x->R;
    x->R = y->L;
    y->L = x;
    recalculate_height(x);
    recalculate_height(y);
    return y;
}

static struct branch* rebalance(struct branch *b) {
    recalculate_height(b);
    int const bal = height(b->L) - height(b->R);
    if (bal > 1) {
        if (height(b->L->R) > height(b->L->L)) {
            b->L = rotate_left(b->L);
        }
        return rotate_right(b);
    }
    if (bal < -1) {
        if (height(b->R->L) > height(b->R->R)) {
            b->R = rotate_right(b->R);
        }
        return rotate_left(b);
    }
    return b;
}

static struct branch* avl_insert(struct branch *root, struct branch *node) {
    if (root) {
        if (node->begin < root->begin) {
            root->L = avl_insert(root->L, node);
        } else {
            root->R = avl_insert(root->R, node);
        }
        return rebalance(root);
    }
    return node;
}

static struct branch* avl_remove_min(struct branch *b, struct branch **out) {
    if (b->L) {
        b->L = avl_remove_min(b->L, out);
        return rebalance(b);
    }
    *out = b;
    return b->R;
}

static struct branch* avl_remove(struct branch *root, int begin) {
    __builtin_assume(root);
    if (begin < root->begin) {
        root->L = avl_remove(root->L, begin);
    } else if (begin > root->begin) {
        root->R = avl_remove(root->R, begin);
    } else {
        struct branch *L_subtree = root->L,
                      *R_subtree = root->R;
        if (R_subtree) {
            struct branch *min;
            R_subtree = avl_remove_min(R_subtree, &min);
            min->L = L_subtree;
            min->R = R_subtree;
            return rebalance(min);
        }
        return L_subtree;
    }
    return rebalance(root);
}

static struct branch* branch_find(struct branch *root, int i) {
    while (root) {
        if (i < root->begin) {
            root = root->L;
        } else if (i >= root->end) {
            root = root->R;
        } else {
            return root;
        }
    }
    return NULL;
}

static struct branch* branch_find_lt(struct branch *root, int i) {
    struct branch *best = NULL;
    while (root) {
        if (i <= root->begin) {
            root = root->L;
        } else {
            best = root;
            root = root->R;
        }
    }
    return best;
}

static struct branch* branch_find_gt(struct branch *root, int i) {
    struct branch *best = NULL;
    while (root) {
        if (i < root->begin) {
            best = root;
            root = root->L;
        } else {
            root = root->R;
        }
    }
    return best;
}

static struct branch* branch_new(int begin, int end, size_t size, int cap) {
    if (cap == 0) {
        cap = next_pow2(end - begin);
    }

    struct branch *b = calloc(1, sizeof *b + (size_t)cap * size);
    b->height = 1;
    b->begin  = begin;
    b->end    = end;
    b->cap    = cap;
    return b;
}

static void* branch_ptr(struct branch *b, size_t size, int i) {
    return b->data + (size_t)(i - b->begin) * size;
}

static struct branch* insert_branch(struct component *c, struct branch *branch) {
    c->root = avl_insert(c->root, branch);
    return branch;
}

static void remove_branch(struct component *c, struct branch *branch) {
    c->root = avl_remove(c->root, branch->begin);
    free(branch);
}

void* component_find(struct component const *c, int i) {
    struct branch *b = branch_find(c->root, i);
    return b ? branch_ptr(b, c->size, i) : NULL;
}

void* component_data(struct component *c, int i) {
    for (void *data = component_find(c,i); data;) {
        return data;
    }

    struct branch *pred = branch_find_lt(c->root, i),
                  *succ = branch_find_gt(c->root, i);

    if (pred && i   == pred->end &&
        succ && i+1 == succ->begin) {
        int const p_len = pred->end - pred->begin,
                  s_len = succ->end - succ->begin;
        struct branch *merge = branch_new(pred->begin, succ->end, c->size, 0);
        memcpy(merge->data                              , pred->data, (size_t)p_len * c->size);
        memcpy(merge->data + (size_t)(p_len+1) * c->size, succ->data, (size_t)s_len * c->size);
        remove_branch(c, pred);
        remove_branch(c, succ);
        return branch_ptr(insert_branch(c, merge), c->size, i);
    }

    if (pred && i == pred->end) {
        int const len = pred->end - pred->begin;
        if (len < pred->cap) {
            pred->end += 1;
            return branch_ptr(pred, c->size, i);
        } else {
            struct branch *grown = branch_new(pred->begin, i+1, c->size, 2*pred->cap);
            memcpy(grown->data, pred->data, (size_t)len * c->size);
            remove_branch(c, pred);
            return branch_ptr(insert_branch(c, grown), c->size, i);
        }
    }

    if (succ && i+1 == succ->begin) {
        int const len = succ->end - succ->begin;
        // TODO: insert into succ directly with memmove() when there's enough cap.
        struct branch *grown = branch_new(i, succ->end, c->size, 0);
        memcpy(grown->data + c->size, succ->data, (size_t)len * c->size);
        remove_branch(c, succ);
        return insert_branch(c, grown)->data;
    }

    return insert_branch(c, branch_new(i,i+1,c->size,1))->data;
}

void component_drop(struct component *c, int i) {
    struct branch *b = branch_find(c->root, i);
    if (b) {
        if (b->begin == i && b->end == i+1) {
            remove_branch(c, b);
            return;
        }

        if (b->begin != i && b->end != i+1) {
            struct branch *L = branch_new(b->begin, i, c->size, 0);
            struct branch *R = branch_new(i+1, b->end, c->size, 0);
            memcpy(L->data, branch_ptr(b,c->size,b->begin), (size_t)(i  -  b->begin) * c->size);
            memcpy(R->data, branch_ptr(b,c->size,     i+1), (size_t)(b->end - (i+1)) * c->size);
            remove_branch(c, b);
            insert_branch(c, L);
            insert_branch(c, R);
            return;
        }

        if (b->begin == i) {
            b->begin += 1;
            memmove(b->data, b->data + c->size, (size_t)(b->end - (i+1)) * c->size);
        } else if (b->end == i+1) {
            b->end -= 1;
        }

        if (b->end - b->begin <= b->cap / 4 && b->cap > 1) {
            struct branch *shrunk = branch_new(b->begin, b->end, c->size, b->cap/2);
            memcpy(shrunk->data, b->data, (size_t)(b->end - b->begin) * c->size);
            remove_branch(c, b);
            insert_branch(c, shrunk);
        }
    }
}

static void each_branch(struct branch *b, void (*fn)(int, void*, void*), void *ctx, size_t size) {
    if (b) {
        each_branch(b->L, fn, ctx, size);
        for (int i = b->begin; i < b->end; i++) {
            fn(i, b->data + (size_t)(i - b->begin) * size, ctx);
        }
        each_branch(b->R, fn, ctx, size);
    }
}

void component_each(struct component const *c, void (*fn)(int, void *data, void *ctx), void *ctx) {
    each_branch(c->root, fn, ctx, c->size);
}

static void branch_free(struct branch *b) {
    if (b) {
        branch_free(b->L);
        branch_free(b->R);
        free(b);
    }
}

void component_free(struct component *c) {
    branch_free(c->root);
    c->root = NULL;
}
