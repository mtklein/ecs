#include "ecs.h"
#include <stdlib.h>
#include <string.h>

static inline void* copy(void *dst, void const *src, size_t len) {
    return len ? memcpy(dst,src,len) : dst;
}

__attribute__((no_sanitize("integer")))
static unsigned hash(int id) {
    unsigned bits = (unsigned)id;
    bits *= 0xcc9e2d51;
    bits = (bits << 15) | (bits >> 17);
    bits *= 0x1b873593;
    return bits;
}

static _Bool is_pow2_or_zero(int x) {
    return (x & (x-1)) == 0;
}

static int find_slot(struct component const *c, int id, int *ix_out) {
    int mask = c->slots - 1;
    unsigned h = hash(id);
    int pos = (int)(h & (unsigned)mask);
    int tomb = -1;
    for (;;) {
        int ix = c->ix[pos];
        if (ix < 0) {
            if (ix == ~0) {
                if (ix_out) { *ix_out = -1; }
                return tomb >= 0 ? tomb : pos;
            }
            if (tomb < 0) { tomb = pos; }
        } else if (c->id[ix] == id) {
            if (ix_out) { *ix_out = ix; }
            return pos;
        }
        pos = (pos + 1) & mask;
    }
}

static void rehash(struct component *c, int slots) {
    int *ix = malloc((size_t)slots * sizeof *ix);
    for (int i = 0; i < slots; i++) {
        ix[i] = ~0;
    }
    int mask = slots - 1;
    for (int i = 0; i < c->n; i++) {
        unsigned h = hash(c->id[i]);
        int pos = (int)(h & (unsigned)mask);
        while (ix[pos] >= 0) {
            pos = (pos + 1) & mask;
        }
        ix[pos] = i;
    }
    free(c->ix);
    c->ix = ix;
    c->slots = slots;
}

void attach(int id, struct component *c, void const *val) {
    if (!c->slots) {
        rehash(c, 2);
    }

    int ix;
    int pos = find_slot(c, id, &ix);
    if (ix >= 0) {
        copy((char*)c->data + (size_t)ix * c->size, val, c->size);
        return;
    }

    if (is_pow2_or_zero(c->n)) {
        size_t const cap = c->n ? 2*(size_t)c->n : 1;
        c->id   = realloc(c->id, cap * sizeof *c->id);
        c->data = c->size ? realloc(c->data, cap * c->size) : c;
    }

    int const new_ix = c->n++;
    c->id[new_ix] = id;
    c->ix[pos] = new_ix;
    copy((char*)c->data + (size_t)new_ix * c->size, val, c->size);

    if (c->n / 3 >= c->slots / 4) {
        rehash(c, c->slots * 2);
    }
}

void detach(int id, struct component *c) {
    if (!c->slots) {
        return;
    }

    int ix;
    int pos = find_slot(c, id, &ix);
    if (ix < 0) {
        return;
    }

    c->ix[pos] = ~1;
    int const back_ix = --c->n;
    int const back_id = c->id[back_ix];
    if (ix != back_ix) {
        c->id[ix] = back_id;
        copy((char*)c->data + (size_t)ix * c->size,
             (char*)c->data + (size_t)back_ix * c->size, c->size);
        int slot = find_slot(c, back_id, NULL);
        c->ix[slot] = ix;
    }
}

void* lookup(int id, struct component const *c) {
    if (!c->slots) {
        return NULL;
    }

    int ix;
    find_slot(c, id, &ix);
    if (ix >= 0) {
        return (char*)c->data + (size_t)ix * c->size;
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
        int found;
        find_slot(lead, *id, &found);
        ix = found + 1;
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
