#include "array.h"
#include "ecs.h"
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

struct pos {
    int id;
    int x,y;
};
static array pos = {.size = sizeof(struct pos)};

struct stats {
    int hp, ac, atk, dmg;
};
static array stats = {.size = sizeof(struct stats)};

struct glyph {
    char ch;
};
static array glyph = {.size = sizeof(struct glyph)};

struct disp {
    enum { LEADER, PARTY, FRIENDLY, NEUTRAL, HOSTILE, MADDENED } kind;
};
static array disp = {.size = sizeof(struct disp)};

struct entity {
    int pos,
        stats,
        glyph,
        disp;
};
static array entity = {.size = sizeof(struct entity)};

static array freelist = {.size = sizeof(int)};

#define ix(id,comp) ((struct entity*)ptr(&entity,id))->comp

#define attach(id, comp, ...) component_set(&comp, &ix(id,comp), &(struct comp){__VA_ARGS__})
#define detach(id, comp)      component_del(&comp, &ix(id,comp))
#define lookup(id, comp)      (struct comp*)component_get(&comp, ix(id,comp))


static int entity_at(int x, int y) {
    for (int ix = 0; ix < pos.n; ix++) {
        struct pos const *p = ptr(&pos, ix);
        if (p == lookup(p->id, pos) && p->x == x && p->y == y) {
            return p->id;
        }
    }
    return 0;
}

static void draw(int w, int h) {
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int const id = entity_at(x,y);

            static char const *color[] = {
                [LEADER]   = "\033[32m",
                [PARTY]    = "\033[32m",
                [FRIENDLY] = "\033[34m",
                [NEUTRAL]  = "\033[33m",
                [HOSTILE]  = "\033[31m",
                [MADDENED] = "\033[35m",
            };

            struct disp  const *d = lookup(id, disp);
            struct glyph const *g = lookup(id, glyph);
            printf("%s%c", d ? color[d->kind] : "\033[0m"
                         , g ? g->ch          : '.');
        }
        printf("\n");
    }
}

static _Bool alive(void) {
    for (int id = 0; id < entity.n; id++) {
        struct disp  const *d = lookup(id, disp);
        struct stats const *s = lookup(id, stats);
        if (d && s) {
            if (d->kind == LEADER && s->hp > 0) {
                return 1;
            }
        }
    }
    return 0;
}

static void combat(int attacker, int defender, int (*d20)(void *ctx), void *ctx) {
    struct stats const *as = lookup(attacker, stats);
    struct stats       *ds = lookup(defender, stats);
    if (as && ds) {
        int const roll = d20(ctx);
        if (roll > 1) {
            if (roll == 20 || roll + as->atk >= ds->ac) {
                ds->hp -= as->dmg;
                if (ds->hp <= 0) {
                    attach(defender, glyph, 'x');
                    detach(defender, stats);
                    detach(defender, disp);
                }
            }
        }
    } else if (as && !ds) {
        drop_id(&entity, &freelist, defender);
    }
}

static void move(int dx, int dy, int w, int h, int (*d20)(void *ctx), void *ctx) {
    for (int id = 0; id < entity.n; id++) {
        struct disp const *d = lookup(id, disp);
        struct pos        *p = lookup(id, pos);
        if (d && p && d->kind == LEADER) {
            int const x = p->x + dx,
                      y = p->y + dy;
            if (x<0 || y<0 || x>=w || y>=h) {
                continue;
            }

            int const found = entity_at(x,y);
            if (found) {
                combat(id,found, d20,ctx);
            } else {
                p->x = x;
                p->y = y;
            }
        }
    }
}

static unsigned rng(unsigned seed) {
    __builtin_mul_overflow(seed, 1103515245u, &seed);
    __builtin_add_overflow(seed,      12345u, &seed);
    return seed;
}
static int d20(void *ctx) {
    unsigned *seed = ctx;
    *seed = rng(*seed);
    return 1 + (int)(*seed % 20);
}

int main(int argc, char const* argv[]) {
    unsigned seed = (unsigned)(argc > 1 ? atoi(argv[1]) : 0);

    (void)alloc_id(&entity, &freelist);

    {
        int const id = alloc_id(&entity, &freelist);
        attach(id, pos  , .id=id, .x=1, .y=1);
        attach(id, stats, .hp=10, .ac=10, .atk=2, .dmg=4);
        attach(id, glyph, '@');
        attach(id, disp , LEADER);
    }

    {
        int const id = alloc_id(&entity, &freelist);
        attach(id, pos  , .id=id, .x=3, .y=1);
        attach(id, stats, .hp=4, .ac=12, .atk=3, .dmg=2);
        attach(id, glyph, 'i');
        attach(id, disp , HOSTILE);
    }

    int const w=10, h=5;

    {
        struct termios termios;
        tcgetattr(STDIN_FILENO, &termios);
        termios.c_lflag &= ~(tcflag_t)(ICANON|ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &termios);
    }

    printf("\033[?1049h");
    printf("\033[?25l");
    while (alive()) {
        printf("\033[H");
        draw(w,h);

        switch (getchar()) {
            case 'q': goto exit;

            case 'h': move(-1,0,w,h, d20,&seed); break;
            case 'j': move(0,+1,w,h, d20,&seed); break;
            case 'k': move(0,-1,w,h, d20,&seed); break;
            case 'l': move(+1,0,w,h, d20,&seed); break;
        }
    }
exit:
    printf("\033[?1049l");
    return 0;
}
