#pragma once

#include "ecs.h"

enum { POS, STATS, GLYPH, DISP, IS_CONTROLLED };

struct pos {
    int x,y;
};

struct stats {
    int hp,ac,atk,dmg;
};

struct glyph {
    char ch;
};

enum disposition : char { INERT, PARTY, FRIENDLY, NEUTRAL, HOSTILE, MADDENED };

struct pixel {
    struct glyph     glyph;
    enum disposition disp;
};
void draw(struct pixel *fb, int w, int h, struct component const*);

int entity_at(int x, int y, struct component const*);

_Bool alive(struct component const*);

void kill(int id, struct component*);

void combat(int attacker, int defender,
            int (*d20)(void *ctx), void *ctx,
            struct component*);

void move(int dx, int dy, int w, int h,
          int (*d20)(void *ctx), void *ctx,
          struct component*);
