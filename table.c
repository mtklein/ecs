#include "table.h"

_Bool lookup_(int id, void *data, struct column const *col[], int cols) {
    char *dst = data;
    for (int i = 0; i < cols; i++) {
        struct column const *c   = col[i];
        size_t        const size = c->vptr->find(c, id, dst);
        if (!size) {
            return 0;
        }
        dst += size;
    }
    return 1;
}

void update_(int id, void const *data, struct column *col[], int cols) {
    char const *src = data;
    for (int i = 0; i < cols; i++) {
        struct column *c = col[i];
        src += c->vptr->attach(c, id, src);
    }
}

void erase_(int id, struct column *col[], int cols) {
    for (int i = 0; i < cols; i++) {
        struct column *c = col[i];
        c->vptr->detach(c, id);
    }
}

_Bool survey_(int *id, void *data, struct column const *col[], int cols) {
    struct column const *guide = *col;
    while (guide->vptr->walk(guide, id)) {
        if (lookup_(*id,data,col,cols)) {
            return 1;
        }
    }
    return 0;
}
