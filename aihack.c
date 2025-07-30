#include "ecs.h"
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

static int const nil = 0;
static int alloc_id(void) {
    static int next = 1;
    return next++;
}

struct pos {int x,y;};
#define component(T) union { T *data; struct component meta; }

static component(struct pos) pos = {.meta.size = sizeof(struct pos)};

struct stats {
    int hp, ac, atk, dmg;
};
static component(struct stats) stats = {.meta.size = sizeof(struct stats)};

static component(char) glyph = {.meta.size = sizeof(char)};

enum disposition { LEADER, PARTY, FRIENDLY, NEUTRAL, HOSTILE, MADDENED };
static component(enum disposition) disp = {.meta.size = sizeof(enum disposition)};

#define set(id,c) component_attach(&(c).meta, id)

static int entity_at(int x, int y) {
    for (int i = 0; i < pos.meta.n; i++) {
        struct pos const *p = pos.data + i;
        if (p->x == x && p->y == y) {
            return pos.meta.id[i];
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

            enum disposition const *d = component_lookup(&disp.meta, id);
            char             const *g = component_lookup(&glyph.meta, id);
            printf("%s%c", d ? color[*d] : "\033[0m"
                         , g ?       *g  : '.');
        }
        printf("\n");
    }
}

static _Bool alive(void) {
    for (int i = 0; i < stats.meta.n; i++) {
        int id = stats.meta.id[i];
        struct stats const *s = stats.data + i;
        enum disposition const *d = component_lookup(&disp.meta, id);
        if (d && *d == LEADER && s->hp > 0) {
            return 1;
        }
    }
    return 0;
}

static void combat(int attacker, int defender, int (*d20)(void *ctx), void *ctx) {
    struct stats const *as = component_lookup(&stats.meta, attacker);
    struct stats       *ds = component_lookup(&stats.meta, defender);
    if (as && ds) {
        int const roll = d20(ctx);
        if (roll > 1) {
            if (roll == 20 || roll + as->atk >= ds->ac) {
                ds->hp -= as->dmg;
                if (ds->hp <= 0) {
                    *(char*)component_attach(&glyph.meta, defender) = 'x';
                    component_detach(&stats.meta, defender);
                    component_detach(&disp.meta, defender);
                }
            }
        }
    } else if (as && !ds) {
        component_detach(&pos.meta, defender);
        component_detach(&glyph.meta, defender);
    }
}

static void move(int dx, int dy, int w, int h, int (*d20)(void *ctx), void *ctx) {
    for (int i = 0; i < pos.meta.n; i++) {
        int id = pos.meta.id[i];
        struct pos *p = pos.data + i;
        enum disposition const *d = component_lookup(&disp.meta, id);
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
        int const id = alloc_id();
        *(struct pos*)set(id, pos)   = (struct pos){1,1};
        *(struct stats*)set(id, stats) = (struct stats){.hp=10, .ac=10, .atk=2, .dmg=4};
        *(char*)set(id, glyph) = '@';
        *(enum disposition*)set(id, disp)  = LEADER;
    }
    {
        int const id = alloc_id();
        ((struct pos*)set(id, pos))->x = 3;
        ((struct pos*)set(id, pos))->y = 1;
        *(struct stats*)set(id, stats) = (struct stats){.hp=4, .ac=12, .atk=3, .dmg=2};
        *(char*)set(id, glyph) = 'i';
        *(enum disposition*)set(id, disp)  = HOSTILE;
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
