#include "ecs.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static inline void* copy(void *dst, void const *src, size_t len) {
    return len ? memcpy(dst,src,len) : dst;
}

__attribute__((no_sanitize("integer")))
static unsigned hash(int id) {
    unsigned bits = (unsigned)id;
    bits ^= bits >> 16;
    bits  = (bits * 0x85ebca6b) & 0xffffffffu;
    bits ^= bits >> 13;
    bits  = (bits * 0xc2b2ae35) & 0xffffffffu;
    bits ^= bits >> 16;
    return bits;
}

static _Bool is_pow2_or_zero(int x) {
    return (x & (x-1)) == 0;
}

static int find_slot(struct component const *c, int id, int *ix) {
    int const mask = c->slots - 1;
    for (int tomb=-1, slot=(int)hash(id) & mask; (_Bool)1; slot = (slot+1) & mask) {
        switch (*ix = c->ix[slot]) {
            case ~0: return tomb >= 0 ? tomb : slot;
            case ~1: if (tomb < 0) { tomb = slot; }        break;
            default: if (c->id[*ix] == id) { return slot;} break;
        }
    }
}

static void rehash(struct component *c, int slots) {
    int   *new_ix = malloc((size_t)slots * sizeof *new_ix);
    memset(new_ix, ~0,     (size_t)slots * sizeof *new_ix);

    int const mask = slots - 1;
    for (int ix = 0; ix < c->n; ix++) {
        int slot = (int)hash(c->id[ix]) & mask;
        while (new_ix[slot] >= 0) {
            slot = (slot+1) & mask;
        }
        new_ix[slot] = ix;
    }
    free(c->ix);
    c->ix    = new_ix;
    c->slots = slots;
}

void attach(int id, struct component *c, void const *val) {
    if (!c->slots) {
        rehash(c, 2);
    }

    int ix;
    int const slot = find_slot(c, id, &ix);
    if (ix >= 0) {
        copy((char*)c->data + (size_t)ix * c->size, val, c->size);
        return;
    }

    if (is_pow2_or_zero(c->n)) {
        size_t const cap = c->n ? 2*(size_t)c->n : 1;
        c->id   = realloc(c->id, cap * sizeof *c->id);
        c->data = c->size ? realloc(c->data, cap * c->size) : c;
    }

    ix = c->n++;
    c->id[ix]   = id;
    c->ix[slot] = ix;
    copy((char*)c->data + (size_t)ix * c->size, val, c->size);

    if (c->n/3 >= c->slots/4) {
        rehash(c, c->slots*2);
    }
}

void detach(int id, struct component *c) {
    if (c->slots) {
        int ix;
        int const slot = find_slot(c, id, &ix);
        if (ix < 0) {
            return;
        }

        c->ix[slot] = ~1;

        int const back_ix = --c->n;
        if (ix != back_ix) {
            int unused;
            int const back_id   = c->id[back_ix],
                      back_slot = find_slot(c,back_id,&unused);
            assert(unused == back_ix);

            c->id[ix] = back_id;
            c->ix[back_slot] = ix;
            copy((char       *)c->data + (size_t)     ix * c->size,
                 (char const *)c->data + (size_t)back_ix * c->size, c->size);
        }
    }
}

void* lookup(int id, struct component const *c) {
    if (c->slots) {
        int ix;
        find_slot(c, id, &ix);
        if (ix >= 0) {
            return (char*)c->data + (size_t)ix * c->size;
        }
    }
    return NULL;
}

void reset(struct component *c) {
    free(c->ix);
    free(c->id);
    if (c->size) {
        free(c->data);
    }
    *c = (struct component){.size=c->size};
}

_Bool join(struct component *c[], int components, int *id, void *vals) {
    struct component const *lead = c[0];
    int ix = 0;
    if (*id >= 0) {
        char const *src = vals;
        for (int i = 0; i < components; i++) {
            copy(lookup(*id, c[i]), src, c[i]->size);
            src += c[i]->size;
        }
        int prev;
        find_slot(lead, *id, &prev);
        ix = prev + 1;
    }
    while (ix < lead->n) {
        *id = lead->id[ix++];
        char *dst = vals;
        for (int i = 0; i < components; i++) {
            void const *src = lookup(*id, c[i]);
            if (!src) {
                goto next_ix;
            }
            copy(dst, src, c[i]->size);
            dst += c[i]->size;
        }
        return 1;

    next_ix: continue;
    }
    return 0;
}
