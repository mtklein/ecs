#include "ecs.h"
#include <stdlib.h>
#include <string.h>

struct branch {
    struct branch *left,*right;
    int  height;
    int  begin,end;
    int  :32;
    char data[];
};

static int height(struct branch *b) {
    return b ? b->height : 0;
}

static void recalculate_height(struct branch *b) {
    int const l = height(b->left ),
              r = height(b->right);
    b->height = (l > r ? l : r) + 1;
}

static struct branch* rotate_right(struct branch *y) {
    struct branch *x = y->left;
    y->left  = x->right;
    x->right = y;
    recalculate_height(y);
    recalculate_height(x);
    return x;
}

static struct branch* rotate_left(struct branch *x) {
    struct branch *y = x->right;
    x->right = y->left;
    y->left  = x;
    recalculate_height(x);
    recalculate_height(y);
    return y;
}

static struct branch* balance(struct branch *b) {
    recalculate_height(b);
    int const bal = height(b->left) - height(b->right);
    if (bal > 1) {
        if (height(b->left->right) > height(b->left->left)) {
            b->left = rotate_left(b->left);
        }
        return rotate_right(b);
    }
    if (bal < -1) {
        if (height(b->right->left) > height(b->right->right)) {
            b->right = rotate_right(b->right);
        }
        return rotate_left(b);
    }
    return b;
}

static struct branch* avl_insert(struct branch *root, struct branch *node) {
    if (root) {
        if (node->begin < root->begin) {
            root->left = avl_insert(root->left, node);
        } else {
            root->right = avl_insert(root->right, node);
        }
        return balance(root);
    }
    return node;
}

static struct branch* avl_remove_min(struct branch *b, struct branch **out) {
    if (b->left) {
        b->left = avl_remove_min(b->left, out);
        return balance(b);
    }
    *out = b;
    return b->right;
}

static struct branch* avl_remove(struct branch *root, int key) {
    if (root) {
        if (key < root->begin) {
            root->left = avl_remove(root->left, key);
        } else if (key > root->begin) {
            root->right = avl_remove(root->right, key);
        } else {
            struct branch *left_subtree = root->left,
                         *right_subtree = root->right;
            if (right_subtree) {
                struct branch *min;
                right_subtree = avl_remove_min(right_subtree, &min);
                min->left  = left_subtree;
                min->right = right_subtree;
                return balance(min);
            }
            return left_subtree;
        }
        return balance(root);
    }
    return NULL;
}

static struct branch* branch_find(struct branch *root, int entity) {
    while (root) {
        if (entity < root->begin) {
            root = root->left;
        } else if (entity >= root->end) {
            root = root->right;
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
            root = root->left;
        } else {
            best = root;
            root = root->right;
        }
    }
    return best;
}

static struct branch* branch_find_gt(struct branch *root, int entity) {
    struct branch *best = NULL;
    while (root) {
        if (entity < root->begin) {
            best = root;
            root = root->left;
        } else {
            root = root->right;
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
        each_branch(b->left, fn, ctx, size);
        for (int i = b->begin; i < b->end; i++) {
            fn(i, b->data + (size_t)(i - b->begin) * size, ctx);
        }
        each_branch(b->right, fn, ctx, size);
    }
}

void component_each(struct component *c, void (*fn)(int entity, void *data, void *ctx), void *ctx) {
    each_branch(c->root, fn, ctx, c->size);
}
