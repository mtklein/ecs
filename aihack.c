#include "ecs.h"
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <time.h>

struct pos { int x,y; };
struct glyph { char ch; };
struct stats { int hp,ac,atk,damage; };

static struct table pos,glyph,stats;
static int next_id = 1;

static int entity_create(void) { return next_id++; }

static void add_pos(int id,int x,int y) { struct pos p={x,y}; table_set(&pos,id,&p); }
static void add_glyph(int id,char ch) { struct glyph g={ch}; table_set(&glyph,id,&g); }
static void add_stats(int id,int hp,int ac,int atk,int dmg) {
    struct stats s={hp,ac,atk,dmg};
    table_set(&stats,id,&s);
}

static void del_entity(int id) {
    table_del(&pos,id);
    table_del(&glyph,id);
    table_del(&stats,id);
}

static int entity_at(int x,int y) {
    struct pos *p=pos.data;
    for (int i=0;i<pos.n;i++) {
        if (p[i].x==x && p[i].y==y) { return pos.key[i]; }
    }
    return 0;
}

static void draw(int w,int h) {
    char *buf = malloc((size_t)w * (size_t)h);
    for (int i = 0; i < w * h; i++) {
        buf[i] = '.';
    }

    struct pos   *p = pos.data;
    struct glyph *g = glyph.data;
    for (int i = 0; i < pos.n; i++) {
        int const id = pos.key[i];
        int const ix = glyph.ix[id];
        if (ix != ~0) {
            int const x = p[i].x;
            int const y = p[i].y;
            if (x >= 0 && y >= 0 && x < w && y < h) {
                buf[y * w + x] = g[ix].ch;
            }
        }
    }

    for (int y = 0; y < h; y++) {
        fwrite(buf + (size_t)y * (size_t)w, 1, (size_t)w, stdout);
        putchar('\n');
    }
    free(buf);
}

static int d20(void) { return 1+rand()%20; }

static void combat(int attacker,int defender) {
    struct stats *as=table_get(&stats,attacker);
    struct stats *ds=table_get(&stats,defender);
    if (!as || !ds) { return; }
    if (d20()+as->atk >= ds->ac) {
        ds->hp -= as->damage;
        if (ds->hp <= 0) { del_entity(defender); }
    }
}

static void move_actor(int id,int dx,int dy,int w,int h) {
    struct pos *p=table_get(&pos,id);
    if (!p) { return; }
    int x=p->x+dx,y=p->y+dy;
    if (x<0 || y<0 || x>=w || y>=h) { return; }
    int target=entity_at(x,y);
    if (target && table_get(&stats,target)) {
        combat(id,target);
    } else if (!target) {
        p->x=x; p->y=y;
    }
}

static int getch(void) {
    struct termios old,new;
    tcgetattr(STDIN_FILENO,&old);
    new=old;
    new.c_lflag &= (tcflag_t)~(ICANON|ECHO);
    tcsetattr(STDIN_FILENO,TCSANOW,&new);
    int c=getchar();
    tcsetattr(STDIN_FILENO,TCSANOW,&old);
    return c;
}

static void seed(void) { srand((unsigned)time(NULL)); }

int main(void) {
    seed();
    pos.size=sizeof(struct pos);
    glyph.size=sizeof(struct glyph);
    stats.size=sizeof(struct stats);
    int player=entity_create();
    add_pos(player,1,1);
    add_glyph(player,'@');
    add_stats(player,10,10,2,4);
    int imp=entity_create();
    add_pos(imp,3,1);
    add_glyph(imp,'i');
    add_stats(imp,4,12,3,2);
    int w=10,h=5;
    for (;;) {
        draw(w,h);
        struct stats *ps=table_get(&stats,player);
        if (!ps || ps->hp<=0) { break; }
        int c=getch();
        if (c=='q') { break; }
        if (c=='h') { move_actor(player,-1,0,w,h); }
        if (c=='j') { move_actor(player,0,1,w,h); }
        if (c=='k') { move_actor(player,0,-1,w,h); }
        if (c=='l') { move_actor(player,1,0,w,h); }
        printf("\033[%dA",h);
    }
    table_drop(&pos);
    table_drop(&glyph);
    table_drop(&stats);
    return 0;
}

/* TODO add table utilities for dense iteration and component masks */

