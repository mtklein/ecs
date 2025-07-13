#pragma once

#include "ecs.h"

struct pos {
    int x,y;
};

struct stats {
    int hp,ac,atk,dmg;
};

struct glyph {
    char ch;
};

void draw(char *fb, int w, int h,
          struct component const *pos,
          struct component const *glyph);

int entity_at(int x, int y,
              struct component const *pos);

_Bool alive(struct component const *stats,
            struct component const *in_party);

void kill(int id,
          struct component *stats,
          struct component *glyph,
          struct component *controlled);

void combat(int attacker, int defender,
            int (*d20)(void *ctx), void *ctx,
            struct component *stats,
            struct component *glyph,
            struct component *controlled);

void move(int dx, int dy, int w, int h,
          int (*d20)(void *ctx), void *ctx,
          struct component *pos,
          struct component *stats,
          struct component *glyph,
          struct component *controlled);
