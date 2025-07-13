#include "systems.h"

void draw(struct pixel *fb, int w, int h,
          struct component const *pos,
          struct component const *glyph,
          struct component const *disp) {
    for (int i = 0; i < w*h; i++) {
        fb[i] = (struct pixel){{'.'}, INERT};
    }
    for (int const *id = pos->id; id < pos->id + pos->n; id++) {
        struct pos       const *p = lookup(*id, pos);
        struct glyph     const *g = lookup(*id, glyph);
        enum disposition const *d = lookup(*id, disp);
        if (g && 0 <= p->x && p->x < w
              && 0 <= p->y && p->y < h) {
            fb[p->y*w + p->x] = (struct pixel){{g->ch}, d ? *d : INERT};
        }
    }
}


int entity_at(int x, int y,
              struct component const *pos) {
    for (int const *id = pos->id; id < pos->id + pos->n; id++) {
        struct pos const *p = lookup(*id, pos);
        if (p->x == x && p->y == y) {
            return *id;
        }
    }
    return 0;
}

_Bool alive(struct component const *stats,
            struct component const *disp) {
    for (int const *id = disp->id; id < disp->id + disp->n; id++) {
        enum disposition *d = lookup(*id, disp);
        if (*d == PARTY) {
            struct stats *s = lookup(*id, stats);
            if (s && s->hp > 0) {
                return 1;
            }
        }
    }
    return 0;
}

void kill(int id,
          struct component *stats,
          struct component *glyph,
          struct component *disp,
          struct component *is_controlled) {
    detach(id, stats);
    attach(id, glyph, &(char){'x'});
    detach(id, disp);
    detach(id, is_controlled);
}

void combat(int attacker, int defender,
            int (*d20)(void *ctx), void *ctx,
            struct component *stats,
            struct component *glyph,
            struct component *disp,
            struct component *is_controlled) {
    struct stats *as = lookup(attacker, stats),
                 *ds = lookup(defender, stats);
    if (as && ds) {
        int const roll = d20(ctx);
        if (roll > 1) {
            if (roll == 20 || roll + as->atk >= ds->ac) {
                ds->hp -= as->dmg;
                if (ds->hp <= 0) {
                    kill(defender, stats,glyph,disp,is_controlled);
                }
            }
        }
    }
}

void move(int dx, int dy, int w, int h,
          int (*d20)(void *ctx), void *ctx,
          struct component *pos,
          struct component *stats,
          struct component *glyph,
          struct component *disp,
          struct component *is_controlled) {
    for (int const *id = is_controlled->id; id < is_controlled->id + is_controlled->n; id++) {
        struct pos *p = lookup(*id, pos);

        int const x = p->x + dx,
                  y = p->y + dy;
        if (x<0 || y<0 || x>=w || y>=h) {
            continue;
        }

        int const found = entity_at(x,y, pos);
        if (found) {
            combat(*id,found, d20,ctx, stats,glyph,disp,is_controlled);
        } else {
            p->x = x;
            p->y = y;
        }
    }
}

