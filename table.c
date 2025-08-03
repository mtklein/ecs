#include "table.h"
#include "ecs.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

static void ensure(struct table *t) {
    if (!t->comp && t->columns) {
        t->comp = calloc((size_t)t->columns, sizeof *t->comp);
    }
}

_Bool lookup(struct table const *t, int id, void *data, ...) {
    va_list ap;
    va_start(ap, data);
    _Bool const ok = vlookup(t, id, data, ap);
    va_end(ap);
    return ok;
}

_Bool vlookup(struct table const *t, int id, void *data, va_list ap) {
    if (!t->comp) {
        return 0;
    }

    size_t off = 0;
    for (;;) {
        int const col = va_arg(ap, int);
        if (col < 0) {
            break;
        }
        void const *src = component_lookup_(t->comp + col, t->column_size[col], id);
        if (!src) {
            return 0;
        }
        memcpy((char*)data + off, src, t->column_size[col]);
        off += t->column_size[col];
    }
    return off > 0;
}

_Bool survey(struct table const *t, int *id, void *data, ...) {
    va_list ap;
    va_start(ap, data);
    _Bool const ok = vsurvey(t, id, data, ap);
    va_end(ap);
    return ok;
}

_Bool vsurvey(struct table const *t, int *id, void *data, va_list ap) {
    if (!t->comp) {
        return 0;
    }

    int *cols = malloc((size_t)t->columns * sizeof *cols);
    int n = 0;
    for (;;) {
        int const col = va_arg(ap, int);
        if (col < 0) {
            break;
        }
        cols[n++] = col;
    }
    if (!n) {
        free(cols);
        return 0;
    }

    struct component const *base = t->comp + cols[0];
    int const last = *id;
    int next = INT_MAX;
    for (int i = 0; i < base->n; i++) {
        int const ent = base->id[i];
        if (ent <= last || ent >= next) {
            continue;
        }
        _Bool ok = 1;
        for (int j = 1; j < n; j++) {
            if (!component_lookup_(t->comp + cols[j], t->column_size[cols[j]], ent)) {
                ok = 0;
                break;
            }
        }
        if (ok) {
            next = ent;
        }
    }
    if (next == INT_MAX) {
        free(cols);
        return 0;
    }

    size_t off = 0;
    for (int j = 0; j < n; j++) {
        void const *src = component_lookup_(t->comp + cols[j], t->column_size[cols[j]], next);
        memcpy((char*)data + off, src, t->column_size[cols[j]]);
        off += t->column_size[cols[j]];
    }
    *id = next;
    free(cols);
    return 1;
}

void update(struct table *t, int id, void const *data, ...) {
    va_list ap;
    va_start(ap, data);
    vupdate(t, id, data, ap);
    va_end(ap);
}

void vupdate(struct table *t, int id, void const *data, va_list ap) {
    ensure(t);
    size_t off = 0;
    for (;;) {
        int const col = va_arg(ap, int);
        if (col < 0) {
            break;
        }
        size_t const size = t->column_size[col];
        void *dst = component_attach_(t->comp + col, size, id);
        memcpy(dst, (char const*)data + off, size);
        off += size;
    }
}

void drop_table(struct table *t) {
    if (t->comp) {
        for (int i = 0; i < t->columns; i++) {
            free(t->comp[i].data);
            free(t->comp[i].id);
            free(t->comp[i].ix);
        }
        free(t->comp);
        t->comp = NULL;
    }
}
