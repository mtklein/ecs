#include "ecs.h"
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

static int next_id = 1;

struct pos {
    int x,y;
};
static struct component pos = {.size=sizeof(struct pos)};

struct glyph {
    char ch;
};
static struct component glyph = {.size=sizeof(struct glyph)};

struct stats {
    int hp,ac,atk,dmg;
};
static struct component stats = {.size=sizeof(struct stats)};

static struct component   in_party = {0};
static struct component controlled = {0};

static void kill(int id) {
    detach(id, &stats);
    detach(id, &in_party);
    detach(id, &controlled);
    attach(id, &glyph, &(char){'x'});
}

static int entity_at(int x, int y) {
    for (int const *id = pos.id; id < pos.id + pos.n; id++) {
        struct pos const *p = lookup(*id, &pos);
        if (p->x == x && p->y == y) {
            return *id;
        }
    }
    return 0;
}

static void draw(char *fb, int w, int h) {
    for (int i = 0; i < w*h; i++) {
        fb[i] = '.';
    }
    for (int const *id = pos.id; id < pos.id + pos.n; id++) {
        struct pos   const *p = lookup(*id, &pos);
        struct glyph const *g = lookup(*id, &glyph);
        if (g && 0 <= p->x && p->x < w
              && 0 <= p->y && p->y < h) {
            fb[p->y*w + p->x] = g->ch;
        }
    }
}

static int d20(void) {
    return 1 + rand()%20;
}

static void combat(int attacker, int defender) {
    struct stats *as = lookup(attacker, &stats),
                 *ds = lookup(defender, &stats);
    if (as && ds && d20() + as->atk >= ds->ac) {
        ds->hp -= as->dmg;
        if (ds->hp <= 0) {
            kill(defender);
        }
    }
}

static void move(int dx, int dy, int w, int h) {
    for (int const *id = controlled.id; id < controlled.id + controlled.n; id++) {
        struct pos *p = lookup(*id, &pos);

        int const x = p->x + dx,
                  y = p->y + dy;
        if (x<0 || y<0 || x>=w || y>=h) {
            continue;
        }

        int const found = entity_at(x,y);
        if (found) {
            combat(*id, found);
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

static _Bool still_alive(void) {
    for (int const *id = in_party.id; id < in_party.id + in_party.n; id++) {
        struct stats *s = lookup(*id, &stats);
        if (s && s->hp <= 0) {
            return 0;
        }
    }
    return 1;
}

int main(int argc, char const* argv[]) {
    srand((unsigned)(argc > 1 ? atoi(argv[1]) : time(NULL)));

    {
        int const player = next_id++;
        attach(player, &pos  , &(struct pos){1,1});
        attach(player, &glyph, &(struct glyph){'@'});
        attach(player, &stats, &(struct stats){.hp=10, .ac=10, .atk=2, .dmg=4});
        attach(player, &in_party  , NULL);
        attach(player, &controlled, NULL);
    }

    {
        int const imp = next_id++;
        attach(imp, &pos  , &(struct pos){3,1});
        attach(imp, &glyph, &(struct glyph){'i'});
        attach(imp, &stats, &(struct stats){.hp=4, .ac=12, .atk=3, .dmg=2});
    }

    int const w=10, h=5;
    char *fb = calloc((size_t)(w*h), sizeof *fb);

    for (_Bool playing = 1; playing; playing &= still_alive()) {
        draw(fb, w,h);

        for (int y = 0; y < h; y++) {
            fwrite(fb + (size_t)(y*w), sizeof *fb, (size_t)w, stdout);
            putchar('\n');
        }

        switch (getch()) {
            case 'q': playing = 0; break;

            case 'h': move(-1,0,w,h); break;
            case 'j': move(0,+1,w,h); break;
            case 'k': move(0,-1,w,h); break;
            case 'l': move(+1,0,w,h); break;
        }
        printf("\033[%dA",h);
    }
    return 0;
}

__attribute__((constructor))
static void premain(void) {
    setenv("LLVM_PROFILE_FILE", "%t/tmp.profraw", 0);
}
