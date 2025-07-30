#include "ecs.h"
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

static int const nil     = 0;
static int       next_id = 1;

struct pos {
    int x,y;
};
struct stats {
    int hp, ac, atk, dmg;
};
enum disposition {
    LEADER, PARTY, FRIENDLY, NEUTRAL, HOSTILE, MADDENED
};

static component(struct pos)       pos;
static component(struct stats)     stats;
static component(char)             glyph;
static component(enum disposition) disp;

#define set(id,c) (*(component_attach(&c, id), c.data + c.ix[id]))
#define get(id,c)    component_lookup(&c, id)
#define del(id,c)    component_detach(&c, id)

static int entity_at(int x, int y) {
    for (int ix = 0; ix < pos.n; ix++) {
        int        const id = pos.id[ix];
        struct pos const *p = pos.data + ix;
        if (p->x == x && p->y == y) {
            return id;
        }
    }
    return nil;
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

            enum disposition const *d = get(id, disp);
            char             const *g = get(id, glyph);
            printf("%s%c", d ? color[*d] : "\033[0m"
                         , g ?       *g  : '.');
        }
        printf("\n");
    }
}

static _Bool alive(void) {
    for (int ix = 0; ix < stats.n; ix++) {
        int              const id = stats.id[ix];
        struct stats     const *s = stats.data + ix;
        enum disposition const *d = get(id, disp);
        if (d && *d == LEADER && s->hp > 0) {
            return 1;
        }
    }
    return 0;
}

static void combat(int attacker, int defender, int (*d20)(void *ctx), void *ctx) {
    struct stats const *as = get(attacker, stats);
    struct stats       *ds = get(defender, stats);
    if (as && ds) {
        int const roll = d20(ctx);
        if (roll > 1) {
            if (roll == 20 || roll + as->atk >= ds->ac) {
                ds->hp -= as->dmg;
                if (ds->hp <= 0) {
                    set(defender, glyph) = 'x';
                    del(defender, stats);
                    del(defender, disp);
                }
            }
        }
    } else if (as && !ds) {
        del(defender, pos);
        del(defender, glyph);
    }
}

static void move(int dx, int dy, int w, int h, int (*d20)(void *ctx), void *ctx) {
    for (int ix = 0; ix < pos.n; ix++) {
        int              const id = pos.id[ix];
        struct pos             *p = pos.data + ix;
        enum disposition const *d = get(id, disp);
        if (d && *d == LEADER) {
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

static void reset_terminal(void) {
    printf("\033[?1049l");
}

int main(int argc, char const* argv[]) {
    int const w=10, h=5;
    unsigned seed = (unsigned)(argc > 1 ? atoi(argv[1]) : 0);

    {
        struct termios termios;
        tcgetattr(STDIN_FILENO, &termios);
        termios.c_lflag &= ~(tcflag_t)(ICANON|ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &termios);
    }
    printf("\033[?1049h");
    printf("\033[?25l");
    atexit(reset_terminal);


    {
        int const id = next_id++;
        set(id, pos)   = (struct pos){1,1};
        set(id, stats) = (struct stats){.hp=10, .ac=10, .atk=2, .dmg=4};
        set(id, glyph) = '@';
        set(id, disp)  = LEADER;
    }
    {
        int const id = next_id++;
        set(id, pos).x = 3;
        set(id, pos).y = 1;
        set(id, stats) = (struct stats){.hp=4, .ac=12, .atk=3, .dmg=2};
        set(id, glyph) = 'i';
        set(id, disp)  = HOSTILE;
    }

    while (alive()) {
        printf("\033[H");
        draw(w,h);

        switch (getchar()) {
            case 'q': return 0;

            case 'h': move(-1,0,w,h, d20,&seed); break;
            case 'j': move(0,+1,w,h, d20,&seed); break;
            case 'k': move(0,-1,w,h, d20,&seed); break;
            case 'l': move(+1,0,w,h, d20,&seed); break;
        }
    }
    return 1;
}
