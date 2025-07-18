#include "ecs.h"
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

static int const none    = 0;
static int       next_id = 1;

struct pos {
    int x,y;
};
static struct component pos = {.size=sizeof(struct pos)};

struct stats {
    int hp, ac, atk, dmg;
};
static struct component stats = {.size=sizeof(struct stats)};

struct glyph {
    char ch;
};
static struct component glyph = {.size=sizeof(struct glyph)};

enum disposition {
    PARTY, FRIENDLY, NEUTRAL, HOSTILE, MADDENED
};
static struct component disp = {.size=sizeof(enum disposition)};

static struct component is_controlled;

static void draw(int *fb, int w, int h) {
    for (int i = 0; i < w*h; i++) {
        fb[i] = none;
    }
    for (int const *id = pos.id; id < pos.id + pos.n; id++) {
        struct pos const *p = lookup(*id, &pos);
        fb[w*p->y + p->x] = *id;
    }

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int const id = fb[y*w + x];

            struct glyph     const *g = lookup(id, &glyph);
            enum disposition const *d = lookup(id, &disp);

            static char const *color[] = {
                [PARTY]    = "\033[32m",
                [FRIENDLY] = "\033[34m",
                [NEUTRAL]  = "\033[33m",
                [HOSTILE]  = "\033[31m",
                [MADDENED] = "\033[35m",
            };
            printf("%s%c", d ? color[*d] : "\033[0m"
                         , g ? g->ch     : '.');
        }
        putchar('\n');
    }
}

static int entity_at(int x, int y) {
    for (int const *id = pos.id; id < pos.id + pos.n; id++) {
        struct pos const *p = lookup(*id, &pos);
        if (p->x == x && p->y == y) {
            return *id;
        }
    }
    return none;
}

static _Bool alive(void) {
    for (int const *id = disp.id; id < disp.id + disp.n; id++) {
        enum disposition const *d = lookup(*id, &disp);
        struct stats     const *s = lookup(*id, &stats);
        if (*d == PARTY && s->hp > 0) {
            return 1;
        }
    }
    return 0;
}

static void kill(int id) {
    detach(id, &stats);
    detach(id, &disp);
    detach(id, &is_controlled);

    attach(id, &glyph, &(char){'x'});
}

static void combat(int attacker, int defender,
                   int (*d20)(void *ctx), void *ctx) {
    struct stats *as = lookup(attacker, &stats),
                 *ds = lookup(defender, &stats);
    if (as && ds) {
        int const roll = d20(ctx);
        if (roll > 1) {
            if (roll == 20 || roll + as->atk >= ds->ac) {
                ds->hp -= as->dmg;
                if (ds->hp <= 0) {
                    kill(defender);
                }
            }
        }
    }
}

static void move(int dx, int dy, int w, int h,
                 int (*d20)(void *ctx), void *ctx) {
    for (int const *id = is_controlled.id; id < is_controlled.id + is_controlled.n; id++) {
        struct pos *p = lookup(*id, &pos);

        int const x = p->x + dx,
                  y = p->y + dy;
        if (x<0 || y<0 || x>=w || y>=h) {
            continue;
        }

        int const found = entity_at(x,y);
        if (found) {
            combat(*id,found, d20,ctx);
        } else {
            p->x = x;
            p->y = y;
        }
    }
}


static int getch(void) {
    struct termios prev;
    tcgetattr(STDIN_FILENO, &prev);

    struct termios quiet = prev;
    quiet.c_lflag &= (tcflag_t)~(ICANON|ECHO);
    tcsetattr(STDIN_FILENO,TCSANOW, &quiet);

    int const c = getchar();
    tcsetattr(STDIN_FILENO,TCSANOW, &prev);

    return c;
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

    {
        int const player = next_id++;
        attach(player, &pos  , &(struct pos){1,1});
        attach(player, &stats, &(struct stats){.hp=10, .ac=10, .atk=2, .dmg=4});
        attach(player, &glyph, &(struct glyph){'@'});
        attach(player, &disp , &(enum disposition){PARTY});
        attach(player, &is_controlled, NULL);
    }

    {
        int const imp = next_id++;
        attach(imp, &pos  , &(struct pos){3,1});
        attach(imp, &stats, &(struct stats){.hp=4, .ac=12, .atk=3, .dmg=2});
        attach(imp, &glyph, &(struct glyph){'i'});
        attach(imp, &disp , &(enum disposition){HOSTILE});
    }

    int const w=10, h=5;
    int *fb = calloc((size_t)(w*h), sizeof *fb);

    while (alive()) {
        draw(fb,w,h);

        switch (getch()) {
            case 'q': return 0;

            case 'h': move(-1,0,w,h, d20,&seed); break;
            case 'j': move(0,+1,w,h, d20,&seed); break;
            case 'k': move(0,-1,w,h, d20,&seed); break;
            case 'l': move(+1,0,w,h, d20,&seed); break;
        }
        printf("\033[%dA",h);
    }
    return 0;
}

__attribute__((constructor))
static void premain(void) {
    setenv("LLVM_PROFILE_FILE", "%t/tmp.profraw", 0);
}
