#include "systems.h"

void draw(struct cell *fb, int w, int h,
          struct component const *pos,
          struct component const *glyph,
          struct component const *disp) {
    for (int i = 0; i < w*h; i++) {
        fb[i] = (struct cell){'.', COLOR_DEFAULT};
    }
    for (int const *id = pos->id; id < pos->id + pos->n; id++) {
        struct pos         const *p = lookup(*id, pos);
        struct glyph       const *g = lookup(*id, glyph);
        struct disposition const *d = lookup(*id, disp);
        if (g && 0 <= p->x && p->x < w
              && 0 <= p->y && p->y < h) {
            char color = COLOR_DEFAULT;
            if (d) {
                if (d->kind == DISPOSITION_IN_PARTY) { color = COLOR_GREEN; }
                else if (d->kind == DISPOSITION_FRIENDLY) { color = COLOR_BLUE; }
                else if (d->kind == DISPOSITION_HOSTILE) { color = COLOR_RED; }
                else if (d->kind == DISPOSITION_NEUTRAL) { color = COLOR_YELLOW; }
                else if (d->kind == DISPOSITION_MADDENED) { color = COLOR_PURPLE; }
            }
            fb[p->y*w + p->x] = (struct cell){g->ch, color};
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
        struct disposition const *d = lookup(*id, disp);
        if (d->kind == DISPOSITION_IN_PARTY) {
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
          struct component *controlled) {
    detach(id, stats);
    attach(id, glyph, &(char){'x'});
    detach(id, controlled);
}

void combat(int attacker, int defender,
            int (*d20)(void *ctx), void *ctx,
            struct component *stats,
            struct component *glyph,
            struct component *controlled) {
    struct stats *as = lookup(attacker, stats),
                 *ds = lookup(defender, stats);
    if (as && ds) {
        int const roll = d20(ctx);
        if (roll > 1) {
            if (roll == 20 || roll + as->atk >= ds->ac) {
                ds->hp -= as->dmg;
                if (ds->hp <= 0) {
                    kill(defender, stats, glyph, controlled);
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
          struct component *controlled) {
    for (int const *id = controlled->id; id < controlled->id + controlled->n; id++) {
        struct pos *p = lookup(*id, pos);

        int const x = p->x + dx,
                  y = p->y + dy;
        if (x<0 || y<0 || x>=w || y>=h) {
            continue;
        }

        int const found = entity_at(x,y, pos);
        if (found) {
            combat(*id,found, d20,ctx, stats,glyph,controlled);
        } else {
            p->x = x;
            p->y = y;
        }
    }
}

