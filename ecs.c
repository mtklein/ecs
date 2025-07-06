#include "ecs.h"
#include <stdlib.h>
#include <string.h>

struct branch {
    struct branch *left;
    struct branch *right;
    int            begin;
    int            end;
    int            height;
    int            :32;
    char           data[];
};

static int height(struct branch *n) { return n ? n->height : 0; }

static void update(struct branch *n) {
    int hl = height(n->left);
    int hr = height(n->right);
    n->height = hl > hr ? hl + 1 : hr + 1;
}

static struct branch *rotate_right(struct branch *y) {
    struct branch *x = y->left;
    y->left = x->right;
    x->right = y;
    update(y);
    update(x);
    return x;
}

static struct branch *rotate_left(struct branch *x) {
    struct branch *y = x->right;
    x->right = y->left;
    y->left = x;
    update(x);
    update(y);
    return y;
}

static struct branch *balance(struct branch *n) {
    update(n);
    int bal = height(n->left) - height(n->right);
    if (bal > 1) {
        if (height(n->left->right) > height(n->left->left)) {
            n->left = rotate_left(n->left);
        }
        return rotate_right(n);
    }
    if (bal < -1) {
        if (height(n->right->left) > height(n->right->right)) {
            n->right = rotate_right(n->right);
        }
        return rotate_left(n);
    }
    return n;
}

static struct branch *avl_insert(struct branch *root, struct branch *node) {
    if (!root) {
        return node;
    }
    if (node->begin < root->begin) {
        root->left = avl_insert(root->left, node);
    } else {
        root->right = avl_insert(root->right, node);
    }
    return balance(root);
}

static struct branch *avl_remove_min(struct branch *n, struct branch **out) {
    if (!n->left) {
        *out = n;
        return n->right;
    }
    n->left = avl_remove_min(n->left, out);
    return balance(n);
}

static struct branch *avl_remove(struct branch *root, int key) {
    if (!root) {
        return NULL;
    }
    if (key < root->begin) {
        root->left = avl_remove(root->left, key);
    } else if (key > root->begin) {
        root->right = avl_remove(root->right, key);
    } else {
        struct branch *left_subtree = root->left;
        struct branch *right_subtree = root->right;
        if (!right_subtree) {
            return left_subtree;
        }
        struct branch *min;
        right_subtree = avl_remove_min(right_subtree, &min);
        min->left = left_subtree;
        min->right = right_subtree;
        return balance(min);
    }
    return balance(root);
}

static struct branch *branch_find(struct branch *root, int entity) {
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

static struct branch *branch_find_lt(struct branch *root, int entity) {
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

static struct branch *branch_find_gt(struct branch *root, int entity) {
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

static struct branch *branch_new(int begin, int end, size_t size) {
    struct branch *n = malloc(sizeof(*n) + (size_t)(end - begin) * size);
    n->left = n->right = NULL;
    n->height = 1;
    n->begin = begin;
    n->end = end;
    memset(n->data, 0, (size_t)(end - begin) * size);
    return n;
}

static void *branch_ptr(struct branch *n, size_t size, int entity) {
    return n->data + (size_t)(entity - n->begin) * size;
}

static struct branch *component_insert_branch(struct component *c, struct branch *branch) {
    c->root = avl_insert(c->root, branch);
    return branch;
}

static void component_remove_branch(struct component *c, struct branch *branch) {
    c->root = avl_remove(c->root, branch->begin);
    free(branch);
}

void *component_data(struct component *c, int entity) {
    struct branch *n = branch_find(c->root, entity);
    if (n) {
        return branch_ptr(n, c->size, entity);
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
            n = merge;
        } else {
            n = newl;
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
            n = merge;
        } else {
            n = newl;
        }
    } else {
        n = branch_new(entity, entity + 1, c->size);
        component_insert_branch(c, n);
    }
    return branch_ptr(n, c->size, entity);
}

void component_drop(struct component *c, int entity) {
    struct branch *n = branch_find(c->root, entity);
    if (!n) {
        return;
    }

    if (n->begin == entity && n->end == entity + 1) {
        component_remove_branch(c, n);
        return;
    }

    if (entity == n->begin) {
        struct branch *newl = branch_new(entity + 1, n->end, c->size);
        memcpy(newl->data, n->data + c->size, (size_t)(n->end - n->begin - 1) * c->size);
        component_remove_branch(c, n);
        component_insert_branch(c, newl);
        return;
    }

    if (entity == n->end - 1) {
        struct branch *newl = branch_new(n->begin, n->end - 1, c->size);
        memcpy(newl->data, n->data, (size_t)(n->end - n->begin - 1) * c->size);
        component_remove_branch(c, n);
        component_insert_branch(c, newl);
        return;
    }

    struct branch *right = branch_new(entity + 1, n->end, c->size);
    memcpy(right->data, branch_ptr(n, c->size, entity + 1),
           (size_t)(n->end - (entity + 1)) * c->size);
    struct branch *left = branch_new(n->begin, entity, c->size);
    memcpy(left->data, n->data, (size_t)(entity - n->begin) * c->size);
    component_remove_branch(c, n);
    component_insert_branch(c, left);
    component_insert_branch(c, right);
}

static void each_branch(struct branch *n,
                      void (*fn)(int, void *, void *),
                      void *ctx,
                      size_t size) {
    if (!n) {
        return;
    }
    each_branch(n->left, fn, ctx, size);
    for (int i = n->begin; i < n->end; ++i) {
        fn(i, n->data + (size_t)(i - n->begin) * size, ctx);
    }
    each_branch(n->right, fn, ctx, size);
}

void component_each(struct component *c,
                    void (*fn)(int entity, void *data, void *ctx),
                    void *ctx) {
    each_branch(c->root, fn, ctx, c->size);
}
