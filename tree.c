#include "tree.h"
#include <stdlib.h>
#include <string.h>

struct tree_node {
    struct tree_node *next;
    int              *id;
    union {
        struct tree_node **child;
        void             *data;
    };
    int    n;
    _Bool  leaf;
    int    :24;
};

static int calc_cap(size_t size) {
    size_t const bytes = 64;
    size_t const elem = sizeof(int) + (size > sizeof(void*) ? size : sizeof(void*));
    int cap = (int)(bytes / elem);
    return cap < 4 ? 4 : cap;
}

static struct tree_node* node_new(struct tree *t, _Bool leaf) {
    struct tree_node *n = malloc(sizeof *n);
    n->n = 0;
    n->leaf = leaf;
    n->id = malloc((size_t)t->cap * sizeof *n->id);
    if (leaf) {
        n->data = t->size ? malloc((size_t)t->cap * t->size) : NULL;
        n->next = NULL;
    } else {
        n->child = malloc((size_t)(t->cap + 1) * sizeof *n->child);
    }
    return n;
}

static void node_free(struct tree_node *n, struct tree *t) {
    if (!n) { return; }
    if (n->leaf) {
        if (t->size) { free(n->data); }
    } else {
        for (int i = 0; i <= n->n; i++) { node_free(n->child[i], t); }
        free(n->child);
    }
    free(n->id);
    free(n);
}

void tree_reset(struct tree *t) {
    node_free(t->root, t);
    *t = (struct tree){.size=t->size};
}

static void split_child(struct tree *t, struct tree_node *parent, int idx) {
    struct tree_node *child = parent->child[idx];
    int const mid = t->cap / 2;
    struct tree_node *new = node_new(t, child->leaf);
    if (child->leaf) {
        new->n = child->n - mid;
        memcpy(new->id, child->id + mid, (size_t)new->n * sizeof *new->id);
        if (t->size) {
            memcpy(new->data,
                   (char*)child->data + (size_t)mid * t->size,
                   (size_t)new->n * t->size);
        }
        child->n = mid;
        new->next = child->next;
        child->next = new;

        for (int j = parent->n; j > idx; j--) {
            parent->child[j+1] = parent->child[j];
            parent->id[j] = parent->id[j-1];
        }
        parent->child[idx+1] = new;
        parent->id[idx] = new->id[0];
        parent->n++;
    } else {
        new->n = child->n - mid - 1;
        memcpy(new->id, child->id + mid + 1, (size_t)new->n * sizeof *new->id);
        memcpy(new->child, child->child + mid + 1,
               (size_t)(new->n + 1) * sizeof *new->child);
        child->n = mid;

        for (int j = parent->n; j > idx; j--) {
            parent->child[j+1] = parent->child[j];
            parent->id[j] = parent->id[j-1];
        }
        parent->child[idx+1] = new;
        parent->id[idx] = child->id[mid];
        parent->n++;
    }
}

static _Bool insert_nonfull(struct tree *t, struct tree_node *node,
                            int id, void const *val) {
    if (node->leaf) {
        int i = node->n;
        while (i > 0 && id < node->id[i-1]) {
            node->id[i] = node->id[i-1];
            if (t->size) {
                memcpy((char*)node->data + (size_t)i * t->size,
                       (char*)node->data + (size_t)(i-1) * t->size,
                       t->size);
            }
            i--;
        }
        if (i > 0 && node->id[i-1] == id) {
            if (t->size) {
                memcpy((char*)node->data + (size_t)(i-1) * t->size,
                       val, t->size);
            }
            return 0;
        }
        node->id[i] = id;
        if (t->size) {
            memcpy((char*)node->data + (size_t)i * t->size, val, t->size);
        }
        node->n++;
        return 1;
    } else {
        int i = node->n;
        while (i > 0 && id < node->id[i-1]) { i--; }
        if (node->child[i]->n == t->cap) {
            split_child(t, node, i);
            if (id >= node->id[i]) { i++; }
        }
        return insert_nonfull(t, node->child[i], id, val);
    }
}

void tree_attach(int id, struct tree *t, void const *val) {
    if (!t->cap) { t->cap = calc_cap(t->size); }
    if (!t->root) { t->root = node_new(t, 1); }
    if (t->root->n == t->cap) {
        struct tree_node *old = t->root;
        t->root = node_new(t, 0);
        t->root->child[0] = old;
        split_child(t, t->root, 0);
    }
    if (insert_nonfull(t, t->root, id, val)) {
        t->n++;
    }
}

void* tree_lookup(int id, struct tree const *t) {
    struct tree_node const *node = t->root;
    while (node && !node->leaf) {
        int i = 0;
        while (i < node->n && id >= node->id[i]) { i++; }
        node = node->child[i];
    }
    if (!node) { return NULL; }
    int i = 0;
    while (i < node->n && node->id[i] < id) { i++; }
    if (i < node->n && node->id[i] == id) {
        return (char*)node->data + (size_t)i * t->size;
    }
    return NULL;
}

void tree_detach(int id, struct tree *t) {
    struct tree_node *node = t->root, *parent = NULL;
    int parent_idx = 0;
    while (node && !node->leaf) {
        int i = 0;
        while (i < node->n && id >= node->id[i]) { i++; }
        parent = node;
        parent_idx = i;
        node = node->child[i];
    }
    if (!node) { return; }
    int i = 0;
    while (i < node->n && node->id[i] < id) { i++; }
    if (i == node->n || node->id[i] != id) { return; }
    if (t->size) {
        memmove((char*)node->data + (size_t)i * t->size,
                (char*)node->data + (size_t)(i+1) * t->size,
                (size_t)(node->n - i - 1) * t->size);
    }
    memmove(node->id + i, node->id + i + 1,
            (size_t)(node->n - i - 1) * sizeof *node->id);
    node->n--;
    t->n--;
    if (parent && i == 0) {
        parent->id[parent_idx] = node->id[0];
    }
    if (node == t->root && node->n == 0) {
        node_free(node, t);
        t->root = NULL;
    }
}
