#include "systems.h"

void draw(char *fb, int w, int h,
          struct component const *pos,
          struct component const *glyph) {
    for (int i = 0; i < w*h; i++) {
        fb[i] = '.';
    }
    for (int const *id = pos->id; id < pos->id + pos->n; id++) {
        struct pos   const *p = lookup(*id, pos);
        struct glyph const *g = lookup(*id, glyph);
        if (g && 0 <= p->x && p->x < w
              && 0 <= p->y && p->y < h) {
            fb[p->y*w + p->x] = g->ch;
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
            struct component const *in_party) {
    for (int const *id = in_party->id; id < in_party->id + in_party->n; id++) {
        struct stats *s = lookup(*id, stats);
        if (s && s->hp > 0) {
            return 1;
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

static enum color entity_color(int id,
                               struct component const *in_party,
                               struct component const *friendly,
                               struct component const *hostile,
                               struct component const *maddened) {
    if (lookup(id, in_party)) {
        return COLOR_GREEN;
    }
    if (lookup(id, friendly)) {
        return COLOR_BLUE;
    }
    if (lookup(id, maddened)) {
        return COLOR_PURPLE;
    }
    if (lookup(id, hostile)) {
        return COLOR_RED;
    }
    return COLOR_DARK_YELLOW;
}

void draw_disposition(char *fb, enum color *cb, int w, int h,
                      struct component const *pos,
                      struct component const *glyph,
                      struct component const *in_party,
                      struct component const *friendly,
                      struct component const *hostile,
                      struct component const *maddened) {
    for (int i = 0; i < w*h; i++) {
        fb[i] = '.';
        cb[i] = COLOR_NONE;
    }
    for (int const *id = pos->id; id < pos->id + pos->n; id++) {
        struct pos   const *p = lookup(*id, pos);
        struct glyph const *g = lookup(*id, glyph);
        if (g && 0 <= p->x && p->x < w
              && 0 <= p->y && p->y < h) {
            int const i = p->y*w + p->x;
            fb[i] = g->ch;
            cb[i] = entity_color(*id, in_party,friendly,hostile,maddened);
        }
    }
}

