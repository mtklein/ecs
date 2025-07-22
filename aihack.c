#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#define MAX_IDS 1024
static int const nil = 0;
static int       ids = 1;
static int  free_ids = 0;
static int  free_id[MAX_IDS];

static struct pos       { int x,y; }                                            pos  [MAX_IDS];
static struct stats     { int hp, ac, atk, dmg; }                               stats[MAX_IDS];
static struct glyph     { char ch; }                                            glyph[MAX_IDS];
static enum disposition { LEADER, PARTY, FRIENDLY, NEUTRAL, HOSTILE, MADDENED } disp [MAX_IDS];

static struct has {
    _Bool pos   : 1,
          stats : 1,
          glyph : 1,
          disp  : 1;
    char        : 4;
} has[MAX_IDS];

static int alloc_id(void) {
    return free_ids ? free_id[--free_ids]
                    : ids++;
}
static void drop_id(int id) {
    has[id] = (struct has){0};
    free_id[free_ids++] = id;
}
#define set(id, comp) comp[has[id].comp = 1, id]
#define get(id, comp) (has[id].comp ? comp+id : NULL)
#define scan(id) for (int id = 0; id < ids; id++)

static int entity_at(int x, int y) {
    scan(id) {
        struct pos const *p = get(id, pos);
        if (p && p->x == x && p->y == y) {
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
            struct glyph const     *g = get(id, glyph);
            printf("%s%c", d ? color[*d] : "\033[0m"
                         , g ? g->ch     : '.');
        }
        printf("\n");
    }
}

static _Bool alive(void) {
    scan(id) {
        enum disposition const *d = get(id, disp);
        struct stats     const *s = get(id, stats);
        if (d && s) {
            if (disp[id] == LEADER && stats[id].hp > 0) {
                return 1;
            }
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
                    struct pos const p = *get(defender, pos);
                    drop_id(defender);

                    int const id = alloc_id();
                    set(id,pos)   = p;
                    set(id,glyph) = (struct glyph){'x'};
                }
            }
        }
    }
}

static void move(int dx, int dy, int w, int h, int (*d20)(void *ctx), void *ctx) {
    scan(id) {
        enum disposition const *d = get(id, disp);
        struct pos             *p = get(id, pos);
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

    {
        int const id = alloc_id();
        set(id, pos)   = (struct pos  ){1,1};
        set(id, stats) = (struct stats){.hp=10, .ac=10, .atk=2, .dmg=4};
        set(id, glyph) = (struct glyph){'@'};
        set(id, disp ) = LEADER;
    }

    {
        int const id = alloc_id();
        set(id, pos  ) = (struct pos  ){3,1};
        set(id, stats) = (struct stats){.hp=4, .ac=12, .atk=3, .dmg=2};
        set(id, glyph) = (struct glyph){'i'};
        set(id, disp ) = HOSTILE;
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
