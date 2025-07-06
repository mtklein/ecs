#include "ecs.h"
#include <stdlib.h>
#include <string.h>

static int height(struct leaf *n) { return n ? n->height : 0; }

static void update(struct leaf *n) {
    int hl = height(n->left);
    int hr = height(n->right);
    n->height = hl > hr ? hl + 1 : hr + 1;
}

static struct leaf *rotate_right(struct leaf *y) {
    struct leaf *x = y->left;
    y->left = x->right;
    x->right = y;
    update(y);
    update(x);
    return x;
}

static struct leaf *rotate_left(struct leaf *x) {
    struct leaf *y = x->right;
    x->right = y->left;
    y->left = x;
    update(x);
    update(y);
    return y;
}

static struct leaf *balance(struct leaf *n) {
    update(n);
    int bal = height(n->left) - height(n->right);
    if (bal > 1) {
        if (height(n->left->right) > height(n->left->left))
            n->left = rotate_left(n->left);
        return rotate_right(n);
    }
    if (bal < -1) {
        if (height(n->right->left) > height(n->right->right))
            n->right = rotate_right(n->right);
        return rotate_left(n);
    }
    return n;
}

static struct leaf *avl_insert(struct leaf *root, struct leaf *node) {
    if (!root) return node;
    if (node->begin < root->begin)
        root->left = avl_insert(root->left, node);
    else
        root->right = avl_insert(root->right, node);
    return balance(root);
}

static struct leaf *avl_remove_min(struct leaf *n, struct leaf **out) {
    if (!n->left) {
        *out = n;
        return n->right;
    }
    n->left = avl_remove_min(n->left, out);
    return balance(n);
}

static struct leaf *avl_remove(struct leaf *root, int key) {
    if (!root) return NULL;
    if (key < root->begin) {
        root->left = avl_remove(root->left, key);
    } else if (key > root->begin) {
        root->right = avl_remove(root->right, key);
    } else {
        struct leaf *l = root->left;
        struct leaf *r = root->right;
        if (!r) {
            /* caller will free */
            return l;
        }
        struct leaf *min;
        r = avl_remove_min(r, &min);
        min->left = l;
        min->right = r;
        return balance(min);
    }
    return balance(root);
}

static struct leaf *leaf_find(struct leaf *root, int entity) {
    while (root) {
        if (entity < root->begin)
            root = root->left;
        else if (entity >= root->end)
            root = root->right;
        else
            return root;
    }
    return NULL;
}

static struct leaf *leaf_find_lt(struct leaf *root, int entity) {
    struct leaf *best = NULL;
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

static struct leaf *leaf_find_gt(struct leaf *root, int entity) {
    struct leaf *best = NULL;
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

static struct leaf *leaf_new(int begin, int end, size_t size) {
    struct leaf *n = malloc(sizeof(*n) + (size_t)(end - begin) * size);
    n->left = n->right = NULL;
    n->height = 1;
    n->begin = begin;
    n->end = end;
    memset(n->data, 0, (size_t)(end - begin) * size);
    return n;
}

static void *leaf_ptr(struct leaf *n, size_t size, int entity) {
    return n->data + (size_t)(entity - n->begin) * size;
}

static struct leaf *component_insert_leaf(struct component *c, struct leaf *leaf) {
    c->root = avl_insert(c->root, leaf);
    return leaf;
}

static void component_remove_leaf(struct component *c, struct leaf *leaf) {
    c->root = avl_remove(c->root, leaf->begin);
    free(leaf);
}

void *component_data(struct component *c, int entity) {
    struct leaf *n = leaf_find(c->root, entity);
    if (n) return leaf_ptr(n, c->size, entity);

    struct leaf *pred = leaf_find_lt(c->root, entity);
    struct leaf *succ = leaf_find_gt(c->root, entity);

    if (pred && entity == pred->end) {
        struct leaf *newl = leaf_new(pred->begin, pred->end + 1, c->size);
        memcpy(newl->data, pred->data, (size_t)(pred->end - pred->begin) * c->size);
        component_remove_leaf(c, pred);
        component_insert_leaf(c, newl);
        if (succ && succ->begin == newl->end) {
            struct leaf *merge = leaf_new(newl->begin, succ->end, c->size);
            memcpy(merge->data, newl->data, (size_t)(newl->end - newl->begin) * c->size);
            memcpy(merge->data + (size_t)(succ->begin - newl->begin) * c->size,
                   succ->data,
                   (size_t)(succ->end - succ->begin) * c->size);
            component_remove_leaf(c, newl);
            component_remove_leaf(c, succ);
            component_insert_leaf(c, merge);
            n = merge;
        } else {
            n = newl;
        }
    } else if (succ && entity + 1 == succ->begin) {
        struct leaf *newl = leaf_new(entity, succ->end, c->size);
        memcpy(newl->data + (size_t)(succ->begin - newl->begin) * c->size,
               succ->data,
               (size_t)(succ->end - succ->begin) * c->size);
        component_remove_leaf(c, succ);
        component_insert_leaf(c, newl);
        if (pred && pred->end == newl->begin) {
            struct leaf *merge = leaf_new(pred->begin, newl->end, c->size);
            memcpy(merge->data,
                   pred->data,
                   (size_t)(pred->end - pred->begin) * c->size);
            memcpy(merge->data + (size_t)(newl->begin - pred->begin) * c->size,
                   newl->data,
                   (size_t)(newl->end - newl->begin) * c->size);
            component_remove_leaf(c, pred);
            component_remove_leaf(c, newl);
            component_insert_leaf(c, merge);
            n = merge;
        } else {
            n = newl;
        }
    } else {
        n = leaf_new(entity, entity + 1, c->size);
        component_insert_leaf(c, n);
    }
    return leaf_ptr(n, c->size, entity);
}

void component_drop(struct component *c, int entity) {
    struct leaf *n = leaf_find(c->root, entity);
    if (!n) return;

    if (n->begin == entity && n->end == entity + 1) {
        component_remove_leaf(c, n);
        return;
    }

    if (entity == n->begin) {
        struct leaf *newl = leaf_new(entity + 1, n->end, c->size);
        memcpy(newl->data, n->data + c->size, (size_t)(n->end - n->begin - 1) * c->size);
        component_remove_leaf(c, n);
        component_insert_leaf(c, newl);
        return;
    }

    if (entity == n->end - 1) {
        struct leaf *newl = leaf_new(n->begin, n->end - 1, c->size);
        memcpy(newl->data, n->data, (size_t)(n->end - n->begin - 1) * c->size);
        component_remove_leaf(c, n);
        component_insert_leaf(c, newl);
        return;
    }

    struct leaf *right = leaf_new(entity + 1, n->end, c->size);
    memcpy(right->data, leaf_ptr(n, c->size, entity + 1),
           (size_t)(n->end - (entity + 1)) * c->size);
    struct leaf *left = leaf_new(n->begin, entity, c->size);
    memcpy(left->data, n->data, (size_t)(entity - n->begin) * c->size);
    component_remove_leaf(c, n);
    component_insert_leaf(c, left);
    component_insert_leaf(c, right);
}

static void each_leaf(struct leaf *n,
                      void (*fn)(int, void *, void *),
                      void *ctx,
                      size_t size) {
    if (!n) return;
    each_leaf(n->left, fn, ctx, size);
    for (int i = n->begin; i < n->end; ++i)
        fn(i, n->data + (size_t)(i - n->begin) * size, ctx);
    each_leaf(n->right, fn, ctx, size);
}

void component_each(struct component *c,
                    void (*fn)(int entity, void *data, void *ctx),
                    void *ctx) {
    each_leaf(c->root, fn, ctx, c->size);
}
