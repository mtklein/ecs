#include "column.h"
#include "table.h"
#include <stdarg.h>

static _Bool vlookup(int id, struct column const *c, va_list ap) {
    while (1) {
        if (!c) {
            return 1;
        }
        if (!c->vptr->find(c,id, va_arg(ap, void*))) {
            return 0;
        }
        c = va_arg(ap, struct column const*);
    }
}

_Bool lookup(int id, ...) {
    va_list ap;
    va_start(ap,id);
    struct column const *c = va_arg(ap, struct column const*);
    _Bool const rc = vlookup(id, c, ap);
    va_end(ap);
    return rc;
}

void update(int id, ...) {
    va_list ap;
    va_start(ap,id);

    while (1) {
        struct column *c = va_arg(ap, struct column*);
        if (!c) {
            va_end(ap);
            return;
        }
        c->vptr->attach(c, id, va_arg(ap, void const*));
    }
}

void erase(int id, ...) {
    va_list ap;
    va_start(ap,id);

    while (1) {
        struct column *c = va_arg(ap, struct column*);
        if (!c) {
            va_end(ap);
            return;
        }
        c->vptr->detach(c, id);
    }
}

_Bool survey(int *id, ...) {
    va_list ap;
    va_start(ap,id);
    struct column const *guide = va_arg(ap, struct column const*);
    while (guide->vptr->walk(guide, id)) {
        if (vlookup(*id,guide,ap)) {
            va_end(ap);
            return 1;
        }
    }
    va_end(ap);
    return 0;
}
