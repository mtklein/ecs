#include "systems.h"

void draw(struct pixel *fb, int w, int h, struct component const *c) {
    for (int i = 0; i < w*h; i++) {
        fb[i] = (struct pixel){{'.'}, INERT};
    }
    for (int const *id = c[POS].id; id < c[POS].id + c[POS].n; id++) {
        struct pos       const *p = lookup(*id, c+POS);
        struct glyph     const *g = lookup(*id, c+GLYPH);
        enum disposition const *d = lookup(*id, c+DISP);
        if (g && 0 <= p->x && p->x < w
              && 0 <= p->y && p->y < h) {
            fb[p->y*w + p->x] = (struct pixel){{g->ch}, d ? *d : INERT};
        }
    }
}


int entity_at(int x, int y, struct component const *c) {
    for (int const *id = c[POS].id; id < c[POS].id + c[POS].n; id++) {
        struct pos const *p = lookup(*id, c+POS);
        if (p->x == x && p->y == y) {
            return *id;
        }
    }
    return 0;
}

_Bool alive(struct component const *c) {
    for (int const *id = c[DISP].id; id < c[DISP].id + c[DISP].n; id++) {
        enum disposition *d = lookup(*id, c+DISP);
        if (*d == PARTY) {
            struct stats *s = lookup(*id, c+STATS);
            if (s && s->hp > 0) {
                return 1;
            }
        }
    }
    return 0;
}

void kill(int id, struct component *c) {
    detach(id, c+STATS);
    attach(id, c+GLYPH, &(char){'x'});
    detach(id, c+DISP);
    detach(id, c+IS_CONTROLLED);
}

void combat(int attacker, int defender,
            int (*d20)(void *ctx), void *ctx,
            struct component *c) {
    struct stats *as = lookup(attacker, c+STATS),
                 *ds = lookup(defender, c+STATS);
    if (as && ds) {
        int const roll = d20(ctx);
        if (roll > 1) {
            if (roll == 20 || roll + as->atk >= ds->ac) {
                ds->hp -= as->dmg;
                if (ds->hp <= 0) {
                    kill(defender, c);
                }
            }
        }
    }
}

void move(int dx, int dy, int w, int h,
          int (*d20)(void *ctx), void *ctx,
          struct component *c) {
    for (int const *id = c[IS_CONTROLLED].id; id < c[IS_CONTROLLED].id + c[IS_CONTROLLED].n; id++) {
        struct pos *p = lookup(*id, c+POS);

        int const x = p->x + dx,
                  y = p->y + dy;
        if (x<0 || y<0 || x>=w || y>=h) {
            continue;
        }

        int const found = entity_at(x,y, c+POS);
        if (found) {
            combat(*id,found, d20,ctx, c);
        } else {
            p->x = x;
            p->y = y;
        }
    }
}

