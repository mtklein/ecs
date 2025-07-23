#include "array.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

static int alloc_id(void) {
    for (int const *id = pop(&freelist); id;) {
        return *id;
    }
    int const id = push(&entity);
    memset(ptr(entity, id), ~0, entity.size);
    return id;
}

static void drop_id(int id) {
    memset(ptr(entity, id), ~0, entity.size);

    int *free_id = ptr(freelist, push(&freelist));
    *free_id = id;
}

#define set(id, comp, ...) \
    set_(&((struct entity*)ptr(entity,id))->comp, &comp, &(struct comp){__VA_ARGS__} )
static void set_(int *ix, array *comp, void const *val) {
    if (*ix < 0) {
        *ix = push(comp);
    }
    memcpy(ptr(*comp, *ix), val, comp->size);
}

#define get(id, comp) ((struct comp*)get_(((struct entity*)ptr(entity,id))->comp, comp))
static void* get_(int ix, array comp) {
    return ix < 0 ? NULL : ptr(comp, ix);
}


static int entity_at(int x, int y) {
    for (int ix = 0; ix < pos.n; ix++) {
        struct pos const *p = ptr(pos, ix);
        if (p == get(p->id, pos) && p->x == x && p->y == y) {
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

            struct disp  const *d = get(id, disp);
            struct glyph const *g = get(id, glyph);
            printf("%s%c", d ? color[d->kind] : "\033[0m"
                         , g ? g->ch          : '.');
        }
        printf("\n");
    }
}

static _Bool alive(void) {
    for (int id = 0; id < entity.n; id++) {
        struct disp  const *d = get(id, disp);
        struct stats const *s = get(id, stats);
        if (d && s) {
            if (d->kind == LEADER && s->hp > 0) {
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
                    struct pos const *p = get(defender, pos),
                                  saved = *p;
                    drop_id(defender);

                    int const id = alloc_id();
                    set(id, pos  , .id=id, .x=saved.x, .y=saved.y);
                    set(id, glyph, 'x');
                }
            }
        }
    }
}

static void move(int dx, int dy, int w, int h, int (*d20)(void *ctx), void *ctx) {
    for (int id = 0; id < entity.n; id++) {
        struct disp const *d = get(id, disp);
        struct pos        *p = get(id, pos);
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

    (void)alloc_id();

    {
        int const id = alloc_id();
        set(id, pos  , .id=id, .x=1, .y=1);
        set(id, stats, .hp=10, .ac=10, .atk=2, .dmg=4);
        set(id, glyph, '@');
        set(id, disp , LEADER);
    }

    {
        int const id = alloc_id();
        set(id, pos  , .id=id, .x=3, .y=1);
        set(id, stats, .hp=4, .ac=12, .atk=3, .dmg=2);
        set(id, glyph, 'i');
        set(id, disp , HOSTILE);
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
