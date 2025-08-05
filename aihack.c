#include "len.h"
#include "sparse_column.h"
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

static struct column *pos;
static struct column *stats;
static struct column *glyph;
static struct column *disp;
static struct column *key_event;
static struct column *attack_event;
static struct column *config_event;

static void drop_columns(void) {
    struct column *col[] = {pos,stats,glyph,disp,key_event,attack_event,config_event};
    for (int i = 0; i < len(col); i++) {
        if (col[i]) {
            col[i]->vptr->drop(col[i]);
        }
    }
}

#define queue_event(col, ...) \
    do { \
        __typeof__((__VA_ARGS__)) tmp = (__VA_ARGS__); \
        update(event_last++, col,&tmp); \
    } while (0)

static int entity_at(int x, int y) {
    struct pos p;
    for (int id = ~0; survey(&id, pos,&p);) {
        if (p.x == x && p.y == y) {
            return id;
        }
    }
    return nil;
}

static struct attack_event try_move(int dx, int dy, int w, int h) {
    struct pos       p;
    enum disposition d;
    for (int id = ~0; survey(&id, pos,&p, disp,&d);) {
        if (d == LEADER) {
            int const x = p.x + dx,
                      y = p.y + dy;
            if (x<0 || y<0 || x>=w || y>=h) {
                break;
            }

            int const found = entity_at(x,y);
            if (found) {
                return (struct attack_event){id, found};
            } else {
                p.x = x;
                p.y = y;
                update(id, pos,&p);
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
    if (lookup(event, config_event,&c)) {
        game_state = c.game_state;
    }

    struct key_event k;
    if (lookup(event, key_event,&k)) {
        if (k.key == 'q') {
            *game_state = QUIT;
        }
    }

    struct stats     s;
    enum disposition d;
    for (int id = ~0; survey(&id, stats,&s, disp,&d);) {
        if (d == LEADER && s.hp <= 0) {
            *game_state = DIED;
        }
    }
}

static void movement_system(int event) {
    static int w,h;

    struct config_event c;
    if (lookup(event, config_event,&c)) {
        w = c.w;
        h = c.h;
    }

    struct key_event k;
    if (lookup(event, key_event,&k)) {
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
            queue_event(attack_event, a);
        }
    }
}

static void combat_system(int event) {
    static int (*d20)(void *rng);
    static void *rng;

    struct config_event c;
    if (lookup(event, config_event,&c)) {
        d20 = c.d20;
        rng = c.rng;
    }

    struct attack_event a;
    if (lookup(event, attack_event,&a)) {
        struct stats as, ds;
        _Bool const has_as = lookup(a.attacker, stats,&as);
        _Bool const has_ds = lookup(a.defender, stats,&ds);
        if (has_as && has_ds) {
            int const roll = d20(rng);
            if (roll > 1) {
                if (roll == 20 || roll + as.atk >= ds.ac) {
                    ds.hp -= as.dmg;
                    if (ds.hp <= 0) {
                        char x = 'x';
                        update(a.defender, glyph,&x);
                        erase(a.defender, stats, disp);
                    } else {
                        update(a.defender, stats,&ds);
                    }
                }
            }
        } else if (has_as && !has_ds) {
            erase(a.defender, pos, glyph);
        }
    }
}

static void draw_system(int event) {
    static int w,h;

    struct config_event c;
    if (lookup(event, config_event,&c)) {
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
            _Bool const has_d = lookup(id, disp ,&d);
            _Bool const has_g = lookup(id, glyph,&g);
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

    pos         = sparse_column(sizeof(struct pos));
    stats       = sparse_column(sizeof(struct stats));
    glyph       = sparse_column(sizeof(char));
    disp        = sparse_column(sizeof(enum disposition));
    key_event   = sparse_column(sizeof(struct key_event));
    attack_event= sparse_column(sizeof(struct attack_event));
    config_event= sparse_column(sizeof(struct config_event));
    atexit(drop_columns);

    {
        struct termios termios;
        tcgetattr(STDIN_FILENO, &termios);
        termios.c_lflag &= ~(tcflag_t)(ICANON|ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &termios);
    }
    printf("\033[?1049h");
    printf("\033[?25l");
    atexit(reset_terminal);

    update(next_id++, pos,   &(struct pos){1,1}
                    , stats, &(struct stats){10,10,2,4}
                    , glyph, &(char){'@'}
                    , disp,  &(enum disposition){LEADER});
    update(next_id++, pos,   &(struct pos){3,1}
                    , stats, &(struct stats){4,12,3,2}
                    , glyph, &(char){'i'}
                    , disp,  &(enum disposition){HOSTILE});

    void (*system[])(int) = {
        game_state_system,
          movement_system,
            combat_system,
              draw_system,
    };

    enum game_state game_state = RUNNING;

    {
        queue_event(config_event, (struct config_event){w,h,&game_state,d20,&seed});
        drain_events(system, len(system));
    }

    while (game_state == RUNNING) {
        queue_event(key_event, (struct key_event){getchar()});
        drain_events(system, len(system));
    }
    return game_state == DIED ? 1 : 0;
}

