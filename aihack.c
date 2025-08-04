#include "len.h"
#include "table.h"
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

static int const nil = 0;
static int next_id = 1;
static int event_first;
static int event_last;

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

enum {
    POS,
    STATS,
    GLYPH,
    DISP,
    KEY_EVENT,
    ATTACK_EVENT,
    CONFIG_EVENT,
};

static size_t const column_size[] = {
    [POS]          = sizeof(struct pos),
    [STATS]        = sizeof(struct stats),
    [GLYPH]        = sizeof(char),
    [DISP]         = sizeof(enum disposition),
    [KEY_EVENT]    = sizeof(struct key_event),
    [ATTACK_EVENT] = sizeof(struct attack_event),
    [CONFIG_EVENT] = sizeof(struct config_event),
};

static struct table world = {
    .column_size = column_size,
    .columns = len(column_size),
};

#define queue_event(col, ...) \
    do { \
        __typeof__((__VA_ARGS__)) tmp = (__VA_ARGS__); \
        update(&world, event_last++, &tmp, col); \
    } while (0)

static int entity_at(int x, int y) {
    struct pos p;
    for (int id = ~0; survey(&world, &id, &p, POS);) {
        if (p.x == x && p.y == y) {
            return id;
        }
    }
    return nil;
}

static struct attack_event try_move(int dx, int dy, int w, int h) {
    struct { struct pos pos; enum disposition disp; } e;
    for (int id = ~0; survey(&world, &id, &e, POS, DISP);) {
        if (e.disp == LEADER) {
            int const x = e.pos.x + dx,
                      y = e.pos.y + dy;
            if (x<0 || y<0 || x>=w || y>=h) {
                break;
            }

            int const found = entity_at(x,y);
            if (found) {
                return (struct attack_event){id, found};
            } else {
                e.pos.x = x;
                e.pos.y = y;
                update(&world, id, &e.pos, POS);
            }
        }
    }
    return (struct attack_event){0};
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

    struct config_event c;
    if (lookup(&world, event, &c, CONFIG_EVENT)) {
        game_state = c.game_state;
    }

    struct key_event k;
    if (lookup(&world, event, &k, KEY_EVENT)) {
        if (k.key == 'q') {
            *game_state = QUIT;
        }
    }

    struct { struct stats stats; enum disposition disp; } e;
    for (int id = ~0; survey(&world, &id, &e, STATS, DISP);) {
        if (e.disp == LEADER && e.stats.hp <= 0) {
            *game_state = DIED;
        }
    }
}

static void movement_system(int event) {
    static int w,h;

    struct config_event c;
    if (lookup(&world, event, &c, CONFIG_EVENT)) {
        w = c.w;
        h = c.h;
    }

    struct key_event k;
    if (lookup(&world, event, &k, KEY_EVENT)) {
        int dx=0, dy=0;
        switch (k.key) {
            default: return;
            case 'h': dx=-1; break;
            case 'j': dy=+1; break;
            case 'k': dy=-1; break;
            case 'l': dx=+1; break;
        }

        struct attack_event a = try_move(dx,dy, w,h);
        if (a.defender) {
            queue_event(ATTACK_EVENT, a);
        }
    }
}

static void combat_system(int event) {
    static int (*d20)(void *rng);
    static void *rng;

    struct config_event c;
    if (lookup(&world, event, &c, CONFIG_EVENT)) {
        d20 = c.d20;
        rng = c.rng;
    }

    struct attack_event a;
    if (lookup(&world, event, &a, ATTACK_EVENT)) {
        struct stats as, ds;
        _Bool const has_as = lookup(&world, a.attacker, &as, STATS);
        _Bool const has_ds = lookup(&world, a.defender, &ds, STATS);
        if (has_as && has_ds) {
            int const roll = d20(rng);
            if (roll > 1) {
                if (roll == 20 || roll + as.atk >= ds.ac) {
                    ds.hp -= as.dmg;
                    if (ds.hp <= 0) {
                        char x = 'x';
                        update(&world, a.defender, &x, GLYPH);
                        erase(&world, a.defender, STATS, DISP);
                    } else {
                        update(&world, a.defender, &ds, STATS);
                    }
                }
            }
        } else if (has_as && !has_ds) {
            erase(&world, a.defender, POS, GLYPH);
        }
    }
}

static void draw_system(int event) {
    static int w,h;

    struct config_event c;
    if (lookup(&world, event, &c, CONFIG_EVENT)) {
        w = c.w;
        h = c.h;
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

            enum disposition d;
            char g;
            _Bool const has_d = lookup(&world, id, &d, DISP);
            _Bool const has_g = lookup(&world, id, &g, GLYPH);
            printf("%s%c", has_d ? color[d] : "\033[0m",
                         has_g ? g      : '.');
        }
        printf("\n");
    }
}

static void drain_events(void (*system[])(int), int systems) {
    for (int event = event_first; event < event_last; event++) {
        for (int i = 0; i < systems; i++) {
            system[i](event);
        }
    }
    event_first = event_last;
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
        struct {
            struct pos pos;
            struct stats stats;
            char glyph;
            int  :24;
        } cols = {{1,1}, {10,10,2,4}, '@'};
        update(&world, id, &cols, POS,STATS,GLYPH);
        enum disposition disp = LEADER;
        update(&world, id, &disp, DISP);
    }
    {
        int const id = next_id++;
        struct {
            struct pos pos;
            struct stats stats;
            char glyph;
            int  :24;
        } cols = {{3,1}, {4,12,3,2}, 'i'};
        update(&world, id, &cols, POS,STATS,GLYPH);
        enum disposition disp = HOSTILE;
        update(&world, id, &disp, DISP);
    }

    void (*system[])(int) = {
        game_state_system,
          movement_system,
            combat_system,
              draw_system,
    };

    enum game_state game_state = RUNNING;

    {
        queue_event(CONFIG_EVENT, (struct config_event){w,h,&game_state,d20,&seed});
        drain_events(system, len(system));
    }

    while (game_state == RUNNING) {
        queue_event(KEY_EVENT, (struct key_event){getchar()});
        drain_events(system, len(system));
    }
    return game_state == DIED ? 1 : 0;
}

