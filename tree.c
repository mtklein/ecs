#include "tree.h"
#include <stdlib.h>
#include <string.h>

static inline void* copy(void *dst, void const *src, size_t len) {
    return len ? memcpy(dst,src,len) : dst;
}

static _Bool is_pow2_or_zero(int x) {
    return (x & (x-1)) == 0;
}

static int find(int id, struct tree const *t) {
    int i = t->root;
    while (i != ~0) {
        int const cur = t->id[i];
        if (id == cur) { return i; }
        i = id < cur ? t->left[i] : t->right[i];
    }
    return ~0;
}

void* tree_lookup(int id, struct tree const *t) {
    int i = find(id,t);
    return i == ~0 ? NULL : (char*)t->data + (size_t)i * t->size;
}

static void grow(struct tree *t) {
    if (is_pow2_or_zero(t->n)) {
        size_t const cap = t->n ? 2*(size_t)t->n : 1;
        t->id     = realloc(t->id,     cap * sizeof *t->id);
        t->left   = realloc(t->left,   cap * sizeof *t->left);
        t->right  = realloc(t->right,  cap * sizeof *t->right);
        t->parent = realloc(t->parent, cap * sizeof *t->parent);
        if (t->size) {
            t->data = realloc(t->data, cap * t->size);
        }
        t->cap = (int)cap;
    }
}

void tree_attach(int id, struct tree *t, void const *val) {
    for (void *dst = tree_lookup(id,t); dst;) {
        copy(dst,val,t->size);
        return;
    }

    grow(t);

    int const ix = t->n++;
    t->id[ix] = id;
    t->left[ix] = t->right[ix] = t->parent[ix] = ~0;
    copy((char*)t->data + (size_t)ix * t->size, val, t->size);

    if (t->root == ~0) {
        t->root = ix;
        return;
    }

    int at = t->root;
    for (;;) {
        if (id < t->id[at]) {
            if (t->left[at] == ~0) {
                t->left[at] = ix;
                t->parent[ix] = at;
                return;
            }
            at = t->left[at];
        } else {
            if (t->right[at] == ~0) {
                t->right[at] = ix;
                t->parent[ix] = at;
                return;
            }
            at = t->right[at];
        }
    }
}

void tree_detach(int id, struct tree *t) {
    int rem = find(id,t);
    if (rem == ~0) {
        return;
    }

    if (t->left[rem] != ~0 && t->right[rem] != ~0) {
        int succ = t->right[rem];
        while (t->left[succ] != ~0) {
            succ = t->left[succ];
        }
        t->id[rem] = t->id[succ];
        copy((char*)t->data + (size_t)rem * t->size,
             (char*)t->data + (size_t)succ * t->size, t->size);
        rem = succ;
    }

    int const child = t->left[rem] != ~0 ? t->left[rem] : t->right[rem];
    int const parent = t->parent[rem];
    if (parent == ~0) {
        t->root = child;
    } else if (t->left[parent] == rem) {
        t->left[parent] = child;
    } else {
        t->right[parent] = child;
    }
    if (child != ~0) {
        t->parent[child] = parent;
    }

    int const last = --t->n;
    if (rem != last) {
        t->id[rem] = t->id[last];
        copy((char*)t->data + (size_t)rem * t->size,
             (char*)t->data + (size_t)last * t->size, t->size);
        t->left[rem]   = t->left[last];
        t->right[rem]  = t->right[last];
        t->parent[rem] = t->parent[last];

        if (t->parent[rem] == ~0) {
            t->root = rem;
        } else if (t->left[t->parent[rem]] == last) {
            t->left[t->parent[rem]] = rem;
        } else if (t->right[t->parent[rem]] == last) {
            t->right[t->parent[rem]] = rem;
        }
        if (t->left[rem] != ~0) { t->parent[t->left[rem]] = rem; }
        if (t->right[rem] != ~0) { t->parent[t->right[rem]] = rem; }
    }
}

void tree_reset(struct tree *t) {
    free(t->left);
    free(t->right);
    free(t->parent);
    free(t->id);
    if (t->size) {
        free(t->data);
    }
    *t = (struct tree){.size=t->size, .root=~0};
}
