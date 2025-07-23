#include "sparse_set.h"
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

static int next_id;

struct pos {
    int x,y;
};
static struct { sparse_set meta; struct pos *data; } pos;

struct stats {
    int hp, ac, atk, dmg;
};
static struct { sparse_set meta; struct stats *data; } stats;

static struct { sparse_set meta; char *data; } glyph;

enum disposition { LEADER, PARTY, FRIENDLY, NEUTRAL, HOSTILE, MADDENED };
static struct { sparse_set meta; enum disposition *data; } disp;

#define attach(id, comp, ...) \
    do { \
        __typeof__(*(comp).data) val = { __VA_ARGS__ }; \
        sparse_set_attach(&(comp).meta, (void**)&(comp).data, sizeof val, id, &val); \
    } while (0)
#define detach(id, comp) \
    sparse_set_detach(&(comp).meta, (void**)&(comp).data, sizeof(*(comp).data), id)
#define lookup(id, comp) \
    ((id) < (comp).meta.sparse_cap && (comp).meta.sparse[id] >= 0 ? \
        &(comp).data[(comp).meta.sparse[id]] : NULL)


static int entity_at(int x, int y) {
    for (int ix = 0; ix < pos.meta.n; ix++) {
        struct pos const *p = &pos.data[ix];
        if (p->x == x && p->y == y) {
            return pos.meta.dense[ix];
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

            enum disposition const *d = lookup(id, disp);
            char const *g = lookup(id, glyph);
            printf("%s%c", d ? color[*d] : "\033[0m"
                         , g ? *g   : '.');
        }
        printf("\n");
    }
}

static _Bool alive(void) {
    for (int id = 0; id < next_id; id++) {
        enum disposition const *d = lookup(id, disp);
        struct stats const *s = lookup(id, stats);
        if (d && s) {
            if (*d == LEADER && s->hp > 0) {
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
        detach(defender, pos);
        detach(defender, glyph);
    }
}

static void move(int dx, int dy, int w, int h, int (*d20)(void *ctx), void *ctx) {
    for (int id = 0; id < next_id; id++) {
        enum disposition const *d = lookup(id, disp);
        struct pos        *p = lookup(id, pos);
        if (d && p && *d == LEADER) {
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

    next_id++;

    {
        int const id = next_id++;
        attach(id, pos  , .x=1, .y=1);
        attach(id, stats, .hp=10, .ac=10, .atk=2, .dmg=4);
        attach(id, glyph, '@');
        attach(id, disp , LEADER);
    }

    {
        int const id = next_id++;
        attach(id, pos  , .x=3, .y=1);
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
