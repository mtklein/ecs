#include "ecs.h"
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#define len(x) (int)(sizeof(x) / sizeof *(x))

static int const nil     = 0;
static int       next_id = 1;
static int       events;

struct pos {
    int x,y;
};
struct stats {
    int hp, ac, atk, dmg;
};
enum disposition {
    LEADER, PARTY, FRIENDLY, NEUTRAL, HOSTILE, MADDENED
};

enum game_state {
    RUNNING, DIED, QUIT
};

static component(struct pos)       pos;
static component(struct stats)     stats;
static component(char)             glyph;
static component(enum disposition) disp;

struct key_event {
    int key;
};

struct attack_event {
    int attacker, defender;
};

struct config_event {
    int w,h;
    enum game_state *game_state;
    int (*d20)(void *rng);
    void *rng;
};

static component(struct    key_event)    key_event;
static component(struct attack_event) attack_event;
static component(struct config_event) config_event;

#define set(id,c) (*component_attach(&c, id))
#define get(id,c)   component_lookup(&c, id)
#define del(id,c)   component_detach(&c, id)

struct recycle {
    struct component *component;
    size_t            size;
};
static component(struct recycle) recycle;

static void* queue_event_(struct component *c, size_t size) {
    int const event = events++;
    set(event, recycle) = (struct recycle){c,size};
    return component_attach_(c, size, event);
}
#define queue_event(c) (*(__typeof__(c.data))queue_event_(&c.type_erased, sizeof *c.data))

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

static struct attack_event try_move(int dx, int dy, int w, int h) {
    struct attack_event result = {0};
    for (int ix = 0; ix < pos.n; ix++) {
        int              const id = pos.id[ix];
        struct pos             *p = pos.data + ix;
        enum disposition const *d = get(id, disp);
        if (d && *d == LEADER) {
            int const x = p->x + dx,
                      y = p->y + dy;
            if (x<0 || y<0 || x>=w || y>=h) {
                break;
            }

            int const found = entity_at(x,y);
            if (found) {
                result.attacker = id;
                result.defender = found;
                break;
            } else {
                p->x = x;
                p->y = y;
            }
        }
    }
    return result;
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

static void game_state_system(int event) {
    static enum game_state *game_state;

    {
        struct config_event const *e = get(event, config_event);
        if (e) {
            game_state = e->game_state;
        }
    }

    {
        struct key_event const *e = get(event, key_event);
        if (e && e->key == 'q') {
            *game_state = QUIT;
        }
    }

    for (int ix = 0; ix < stats.n; ix++) {
        int              const id = stats.id[ix];
        struct stats     const *s = stats.data + ix;
        enum disposition const *d = get(id, disp);
        if (d && *d == LEADER && s->hp <= 0) {
            *game_state = DIED;
        }
    }
}

static void movement_system(int event) {
    static int w,h;

    {
        struct config_event const *e = get(event, config_event);
        if (e) {
            w = e->w;
            h = e->h;
        }
    }

    {
        struct key_event const *e = get(event, key_event);
        if (e) {
            int dx=0, dy=0;
            switch (e->key) {
                default: return;
                case 'h': dx=-1; break;
                case 'j': dy=+1; break;
                case 'k': dy=-1; break;
                case 'l': dx=+1; break;
            }

            struct attack_event a = try_move(dx,dy, w,h);
            if (a.defender) {
                queue_event(attack_event) = a;
            }
        }
    }
}

static void combat_system(int event) {
    static int (*d20)(void *rng);
    static void *rng;

    {
        struct config_event const *e = get(event, config_event);
        if (e) {
            d20 = e->d20;
            rng = e->rng;
        }
    }

    {
        struct attack_event const *e = get(event, attack_event);
        if (e) {
            struct stats const *as = get(e->attacker, stats);
            struct stats       *ds = get(e->defender, stats);
            if (as && ds) {
                int const roll = d20(rng);
                if (roll > 1) {
                    if (roll == 20 || roll + as->atk >= ds->ac) {
                        ds->hp -= as->dmg;
                        if (ds->hp <= 0) {
                            set(e->defender, glyph) = 'x';
                            del(e->defender, stats);
                            del(e->defender, disp);
                        }
                    }
                }
            } else if (as && !ds) {
                del(e->defender, pos);
                del(e->defender, glyph);
            }
        }
    }
}

static void draw_system(int event) {
    static int w,h;

    {
        struct config_event const *e = get(event, config_event);
        if (e) {
            w = e->w;
            h = e->h;
        }
    }

    printf("\033[H");
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

static void drain_events(void (*system[])(int), int systems) {
    for (int event = 0; event < events; event++) {
        for (int i = 0; i < systems; i++) {
            system[i](event);
        }

        struct recycle *r = get(event, recycle);
        component_detach_(r->component, r->size, event);
        del(event, recycle);
    }
    events = 0;
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

    void (*system[])(int) = {
        game_state_system,
          movement_system,
            combat_system,
              draw_system,
    };

    enum game_state game_state = RUNNING;

    {
        queue_event(config_event) = (struct config_event){w,h,&game_state,d20,&seed};
        drain_events(system, len(system));
    }

    while (game_state == RUNNING) {
        queue_event(key_event).key = getchar();
        drain_events(system, len(system));
    }
    return game_state == DIED ? 1 : 0;
}
