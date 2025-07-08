#include "ecs.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#define len(x) (int)(sizeof x / sizeof *x)

static int next_id = 1;

struct pos {
    int x,y;
};
static struct component pos_ = {.size=sizeof(struct pos)}, *pos=&pos_;

struct glyph {
    char ch;
};
static struct component glyph_ = {.size=sizeof(struct glyph)}, *glyph=&glyph_;

struct stats {
    int hp,ac,atk,dmg;
};
static struct component stats_ = {.size=sizeof(struct stats)}, *stats=&stats_;

enum { log_lines_cap = 5, log_cols = 32 };
static int log_first, log_len;
static char log_lines_buf[log_lines_cap][log_cols];

static void log_event(char const *msg) {
    int const ix = (log_first + log_len) % log_lines_cap;
    strncpy(log_lines_buf[ix], msg, log_cols - 1);
    log_lines_buf[ix][log_cols - 1] = '\0';
    if (log_len < log_lines_cap) {
        log_len++;
    } else {
        log_first = (log_first + 1) % log_lines_cap;
    }
}

static int entity_at(int x, int y) {
    struct pos const *p = pos->data;
    for (int i = 0; i < pos->n; i++) {
        if (p[i].x == x && p[i].y == y) {
            return pos->id[i];
        }
    }
    return 0;
}

static void draw(char *fb, int w, int h) {
    for (int i = 0; i < w*h; i++) {
        fb[i] = '.';
    }

    struct component *query[] = {pos,glyph};
    struct __attribute__((packed)) {
        struct pos   pos;
        struct glyph glyph;
    } vals;

    for (int id=~0; join(query,len(query), &id,&vals);) {
        int const x = vals.pos.x,
                  y = vals.pos.y;
        if (1 && 0 <= x && x < w
              && 0 <= y && y < h) {
            fb[y*w + x] = vals.glyph.ch;
        }
    }
}

static void render(char *fb, int w, int h) {
    int idx = log_first;
    for (int y = 0; y < h; y++) {
        fwrite(fb + (size_t)(y*w), sizeof *fb, (size_t)w, stdout);
        putchar(' ');
        char const *msg = "";
        if (y < log_len) {
            msg = log_lines_buf[idx];
            idx = (idx + 1) % log_lines_cap;
        }
        printf("%-*s\n", log_cols - 1, msg);
    }
}

static int d20(void) {
    int const r = 1 + rand()%20;
    return r;
}

static void combat(int attacker, int defender) {
    struct stats *as = lookup(attacker, stats),
                 *ds = lookup(defender, stats);
    struct glyph *ag = lookup(attacker, glyph),
                 *dg = lookup(defender, glyph);

    int const roll = d20();
    char msg[log_cols];
    snprintf(msg, sizeof msg, "%c attacks %c", ag->ch, dg->ch);
    log_event(msg);
    snprintf(msg, sizeof msg, "roll %d +%d vs %d", roll, as->atk, ds->ac);
    log_event(msg);

    if (roll + as->atk >= ds->ac) {
        snprintf(msg, sizeof msg, "hit for %d", as->dmg);
        log_event(msg);
        ds->hp -= as->dmg;
        if (ds->hp <= 0) {
            snprintf(msg, sizeof msg, "%c dies", dg->ch);
            log_event(msg);
            detach(defender, pos);
            detach(defender, glyph);
            detach(defender, stats);
        }
    } else {
        log_event("miss");
    }
}

static void move_actor(int actor, int dx, int dy, int w, int h) {
    struct pos *p = lookup(actor, pos);

    int const x = p->x + dx,
              y = p->y + dy;
    if (x<0 || y<0 || x>=w || y>=h) {
        return;
    }

    int const target = entity_at(x,y);
    if (lookup(target, stats)) {
        combat(actor, target);
    } else if (!target) {
        p->x = x;
        p->y = y;
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

int main(int argc, char const* argv[]) {
    srand((unsigned)(argc > 1 ? atoi(argv[1]) : time(NULL)));

    int const player = next_id++;
    attach(player, pos  , &(struct pos){1,1});
    attach(player, glyph, &(struct glyph){'@'});
    attach(player, stats, &(struct stats){.hp=10, .ac=10, .atk=2, .dmg=4});

    int const imp = next_id++;
    attach(imp, pos  , &(struct pos){3,1});
    attach(imp, glyph, &(struct glyph){'i'});
    attach(imp, stats, &(struct stats){.hp=4, .ac=12, .atk=3, .dmg=2});

    int const w=10, h=5;
    char *fb = calloc((size_t)(w*h), sizeof *fb);

    for (_Bool done = 0; !done;) {
        draw(fb, w,h);
        render(fb, w,h);

        struct stats *ps = lookup(player, stats);
        if (ps->hp <= 0) {
            done = 1;
        }

        switch (getch()) {
            case 'q': done = 1; break;

            case 'h': move_actor(player,-1,0,w,h); break;
            case 'j': move_actor(player,0,+1,w,h); break;
            case 'k': move_actor(player,0,-1,w,h); break;
            case 'l': move_actor(player,+1,0,w,h); break;
        }
        printf("\033[%dA",h);
    }

    reset(pos);
    reset(glyph);
    reset(stats);
    return 0;
}

/* TODO add component utilities for dense iteration and component masks */

