#include "pma.h"
#include <stdlib.h>
#include <string.h>

static inline void* pcopy(void *dst, void const *src, size_t len) {
    return len ? memcpy(dst,src,len) : dst;
}

static int search(struct pma const *p, int id, int *found) {
    int lo = 0, hi = p->n;
    while (lo < hi) {
        int const mid = (lo + hi) / 2;
        int const mid_id = p->id[mid];
        if (id < mid_id) {
            hi = mid;
        } else if (id > mid_id) {
            lo = mid + 1;
        } else {
            *found = 1;
            return mid;
        }
    }
    *found = 0;
    return lo;
}

void pma_attach(int id, struct pma *p, void const *val) {
    int found;
    int const ix = search(p,id,&found);
    if (found) {
        pcopy((char*)p->data + (size_t)ix * p->size, val, p->size);
        return;
    }
    if (p->n == p->cap) {
        int const cap = p->cap ? 2*p->cap : 1;
        p->id   = realloc(p->id, (size_t)cap * sizeof *p->id);
        p->data = p->size ? realloc(p->data, (size_t)cap * p->size) : p;
        p->cap  = cap;
    }
    memmove(p->id+ix+1, p->id+ix, (size_t)(p->n-ix) * sizeof *p->id);
    if (p->size) {
        memmove((char*)p->data + (size_t)(ix+1)*p->size,
                (char*)p->data + (size_t)ix   *p->size,
                (size_t)(p->n-ix)*p->size);
    }
    p->id[ix] = id;
    pcopy((char*)p->data + (size_t)ix * p->size, val, p->size);
    p->n++;
}

void pma_detach(int id, struct pma *p) {
    int found;
    int const ix = search(p,id,&found);
    if (!found) { return; }
    memmove(p->id+ix, p->id+ix+1, (size_t)(p->n-ix-1) * sizeof *p->id);
    if (p->size) {
        memmove((char*)p->data + (size_t)ix   *p->size,
                (char*)p->data + (size_t)(ix+1)*p->size,
                (size_t)(p->n-ix-1)*p->size);
    }
    p->n--;
}

void* pma_lookup(int id, struct pma const *p) {
    int found;
    int const ix = search(p,id,&found);
    if (found) {
        return (char*)p->data + (size_t)ix * p->size;
    }
    return NULL;
}

void pma_reset(struct pma *p) {
    free(p->id);
    if (p->size) {
        free(p->data);
    }
    *p = (struct pma){.size=p->size};
}
