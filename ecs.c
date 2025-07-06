#include "ecs.h"
#include <stdlib.h>
#include <string.h>

struct branch {
    struct branch *L,*R;
    int  height;
    int  begin,end;
    int  :32;
    char data[];
};

static int height(struct branch *b) {
    return b ? b->height : 0;
}

static void recalculate_height(struct branch *b) {
    int const l = height(b->L),
              r = height(b->R);
    b->height = (l > r ? l : r) + 1;
}

static struct branch* rotate_right(struct branch *y) {
    struct branch *x = y->L;
    y->L  = x->R;
    x->R = y;
    recalculate_height(y);
    recalculate_height(x);
    return x;
}

static struct branch* rotate_left(struct branch *x) {
    struct branch *y = x->R;
    x->R = y->L;
    y->L  = x;
    recalculate_height(x);
    recalculate_height(y);
    return y;
}

static struct branch* balance(struct branch *b) {
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
        return balance(root);
    }
    return node;
}

static struct branch* avl_remove_min(struct branch *b, struct branch **out) {
    if (b->L) {
        b->L = avl_remove_min(b->L, out);
        return balance(b);
    }
    *out = b;
    return b->R;
}

static struct branch* avl_remove(struct branch *root, int key) {
    if (root) {
        if (key < root->begin) {
            root->L = avl_remove(root->L, key);
        } else if (key > root->begin) {
            root->R = avl_remove(root->R, key);
        } else {
            struct branch *L_subtree = root->L,
                         *R_subtree = root->R;
            if (R_subtree) {
                struct branch *min;
                R_subtree = avl_remove_min(R_subtree, &min);
                min->L = L_subtree;
                min->R = R_subtree;
                return balance(min);
            }
            return L_subtree;
        }
        return balance(root);
    }
    return NULL;
}

static struct branch* branch_find(struct branch *root, int entity) {
    while (root) {
        if (entity < root->begin) {
            root = root->L;
        } else if (entity >= root->end) {
            root = root->R;
        } else {
            return root;
        }
    }
    return NULL;
}

static struct branch* branch_find_lt(struct branch *root, int entity) {
    struct branch *best = NULL;
    while (root) {
        if (entity <= root->begin) {
            root = root->L;
        } else {
            best = root;
            root = root->R;
        }
    }
    return best;
}

static struct branch* branch_find_gt(struct branch *root, int entity) {
    struct branch *best = NULL;
    while (root) {
        if (entity < root->begin) {
            best = root;
            root = root->L;
        } else {
            root = root->R;
        }
    }
    return best;
}

static struct branch* branch_new(int begin, int end, size_t size) {
    struct branch *b = calloc(1, sizeof *b + (size_t)(end - begin) * size);
    b->height = 1;
    b->begin  = begin;
    b->end    = end;
    return b;
}

static void* branch_ptr(struct branch *b, size_t size, int entity) {
    return b->data + (size_t)(entity - b->begin) * size;
}

static struct branch* component_insert_branch(struct component *c, struct branch *branch) {
    c->root = avl_insert(c->root, branch);
    return branch;
}

static void component_remove_branch(struct component *c, struct branch *branch) {
    c->root = avl_remove(c->root, branch->begin);
    free(branch);
}

void* component_data(struct component *c, int entity) {
    struct branch *b = branch_find(c->root, entity);
    if (b) {
        return branch_ptr(b, c->size, entity);
    }

    struct branch *pred = branch_find_lt(c->root, entity);
    struct branch *succ = branch_find_gt(c->root, entity);

    if (pred && entity == pred->end) {
        struct branch *newl = branch_new(pred->begin, pred->end + 1, c->size);
        memcpy(newl->data, pred->data, (size_t)(pred->end - pred->begin) * c->size);
        component_remove_branch(c, pred);
        component_insert_branch(c, newl);
        if (succ && succ->begin == newl->end) {
            struct branch *merge = branch_new(newl->begin, succ->end, c->size);
            memcpy(merge->data, newl->data, (size_t)(newl->end - newl->begin) * c->size);
            memcpy(merge->data + (size_t)(succ->begin - newl->begin) * c->size,
                   succ->data,
                   (size_t)(succ->end - succ->begin) * c->size);
            component_remove_branch(c, newl);
            component_remove_branch(c, succ);
            component_insert_branch(c, merge);
            b = merge;
        } else {
            b = newl;
        }
    } else if (succ && entity + 1 == succ->begin) {
        struct branch *newl = branch_new(entity, succ->end, c->size);
        memcpy(newl->data + (size_t)(succ->begin - newl->begin) * c->size,
               succ->data,
               (size_t)(succ->end - succ->begin) * c->size);
        component_remove_branch(c, succ);
        component_insert_branch(c, newl);
        if (pred && pred->end == newl->begin) {
            struct branch *merge = branch_new(pred->begin, newl->end, c->size);
            memcpy(merge->data,
                   pred->data,
                   (size_t)(pred->end - pred->begin) * c->size);
            memcpy(merge->data + (size_t)(newl->begin - pred->begin) * c->size,
                   newl->data,
                   (size_t)(newl->end - newl->begin) * c->size);
            component_remove_branch(c, pred);
            component_remove_branch(c, newl);
            component_insert_branch(c, merge);
            b = merge;
        } else {
            b = newl;
        }
    } else {
        b = branch_new(entity, entity + 1, c->size);
        component_insert_branch(c, b);
    }
    return branch_ptr(b, c->size, entity);
}

void component_drop(struct component *c, int entity) {
    struct branch *b = branch_find(c->root, entity);
    if (b) {
        if (b->begin == entity && b->end == entity + 1) {
            component_remove_branch(c, b);
            return;
        }

        if (entity == b->begin) {
            struct branch *newl = branch_new(entity + 1, b->end, c->size);
            memcpy(newl->data, b->data + c->size, (size_t)(b->end - b->begin - 1) * c->size);
            component_remove_branch(c, b);
            component_insert_branch(c, newl);
            return;
        }

        if (entity == b->end - 1) {
            struct branch *newl = branch_new(b->begin, b->end - 1, c->size);
            memcpy(newl->data, b->data, (size_t)(b->end - b->begin - 1) * c->size);
            component_remove_branch(c, b);
            component_insert_branch(c, newl);
            return;
        }

        struct branch *right = branch_new(entity + 1, b->end, c->size);
        memcpy(right->data, branch_ptr(b, c->size, entity + 1),
               (size_t)(b->end - (entity + 1)) * c->size);
        struct branch *left = branch_new(b->begin, entity, c->size);
        memcpy(left->data, b->data, (size_t)(entity - b->begin) * c->size);
        component_remove_branch(c, b);
        component_insert_branch(c, left);
        component_insert_branch(c, right);
    }
}

static void each_branch(struct branch *b, void (*fn)(int, void *, void *), void *ctx, size_t size) {
    if (b) {
        each_branch(b->L, fn, ctx, size);
        for (int i = b->begin; i < b->end; i++) {
            fn(i, b->data + (size_t)(i - b->begin) * size, ctx);
        }
        each_branch(b->R, fn, ctx, size);
    }
}

void component_each(struct component *c, void (*fn)(int entity, void *data, void *ctx), void *ctx) {
    each_branch(c->root, fn, ctx, c->size);
}
