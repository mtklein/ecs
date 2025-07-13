#include "systems.h"
#include "test.h"

static int constant_roll(void *ctx) {
    return *(int *)ctx;
}

test(draw) {
    enum { W = 3, H = 2 };
    int const w = W, h = H;
    struct cell fb[W*H];

    __attribute__((cleanup(reset)))
    struct component pos   = {.size = sizeof(struct pos)},
                     glyph = {.size = sizeof(struct glyph)},
                     disp  = {.size = sizeof(struct disposition)};

    attach(1, &pos, &(struct pos){0,0});
    attach(1, &glyph, &(struct glyph){'a'});
    attach(2, &pos, &(struct pos){2,1});
    attach(2, &glyph, &(struct glyph){'b'});
    attach(1, &disp, &(struct disposition){DISPOSITION_IN_PARTY});
    attach(2, &disp, &(struct disposition){DISPOSITION_HOSTILE});
    attach(3, &pos, &(struct pos){4,4});
    attach(3, &glyph, &(struct glyph){'c'});

    draw(fb,w,h,&pos,&glyph,&disp);

    expect(fb[0].ch == 'a' && fb[0].color == COLOR_GREEN);
    expect(fb[5].ch == 'b' && fb[5].color == COLOR_RED);
    for (int i = 0; i < w*h; i++) {
        if (i == 0 || i == 5) { continue; }
        expect(fb[i].ch == '.' && fb[i].color == COLOR_DEFAULT);
    }
}

test(entity_at) {
    __attribute__((cleanup(reset)))
    struct component pos = {.size = sizeof(struct pos)};
    attach(1, &pos, &(struct pos){1,0});
    attach(2, &pos, &(struct pos){2,1});

    expect(entity_at(1,0,&pos) == 1);
    expect(entity_at(2,1,&pos) == 2);
    expect(entity_at(0,0,&pos) == 0);
}

test(alive) {
    __attribute__((cleanup(reset)))
    struct component stats = {.size = sizeof(struct stats)},
                     disp  = {.size = sizeof(struct disposition)};

    attach(1, &stats, &(struct stats){.hp=0});
    attach(1, &disp, &(struct disposition){DISPOSITION_IN_PARTY});
    attach(2, &stats, &(struct stats){.hp=5});
    attach(2, &disp, &(struct disposition){DISPOSITION_IN_PARTY});

    expect(alive(&stats,&disp));

    struct stats *s = lookup(2, &stats);
    s->hp = 0;
    expect(!alive(&stats,&disp));
}

test(kill) {
    __attribute__((cleanup(reset)))
    struct component stats = {.size = sizeof(struct stats)},
                     glyph = {.size = sizeof(struct glyph)},
                     ctrl  = {0};

    attach(1, &stats, &(struct stats){.hp=5});
    attach(1, &glyph, &(struct glyph){'@'});
    attach(1, &ctrl, NULL);

    kill(1,&stats,&glyph,&ctrl);

    expect(!lookup(1,&stats));
    struct glyph *g = lookup(1,&glyph);
    expect(g && g->ch == 'x');
    expect(!lookup(1,&ctrl));
}

test(combat) {
    __attribute__((cleanup(reset)))
    struct component stats = {.size = sizeof(struct stats)},
                     glyph = {.size = sizeof(struct glyph)},
                     ctrl  = {0};

    int attacker = 1;
    int defender = 2;

    attach(attacker,&stats,&(struct stats){.hp=10,.ac=10,.atk=2,.dmg=4});
    attach(defender,&stats,&(struct stats){.hp=3,.ac=0,.atk=0,.dmg=0});
    attach(defender,&glyph,&(struct glyph){'e'});

    int roll = 10;
    combat(attacker,defender, constant_roll,&roll, &stats,&glyph,&ctrl);

    expect(!lookup(defender,&stats));
    struct glyph *g = lookup(defender,&glyph);
    expect(g && g->ch == 'x');
}

test(move) {
    int const w = 5, h = 5;
    __attribute__((cleanup(reset)))
    struct component pos   = {.size = sizeof(struct pos)},
                     stats = {.size = sizeof(struct stats)},
                     glyph = {.size = sizeof(struct glyph)},
                     ctrl  = {0};

    int player = 1;
    attach(player,&pos,&(struct pos){1,1});
    attach(player,&stats,&(struct stats){.hp=10,.ac=10,.atk=20,.dmg=5});
    attach(player,&glyph,&(struct glyph){'@'});
    attach(player,&ctrl,NULL);

    int enemy = 2;
    attach(enemy,&pos,&(struct pos){3,1});
    attach(enemy,&stats,&(struct stats){.hp=4,.ac=0,.atk=0,.dmg=0});
    attach(enemy,&glyph,&(struct glyph){'e'});

    int roll = 10;
    move(1,0,w,h, constant_roll,&roll, &pos,&stats,&glyph,&ctrl);
    struct pos *p = lookup(player,&pos);
    expect(p->x == 2 && p->y == 1);

    move(1,0,w,h, constant_roll,&roll, &pos,&stats,&glyph,&ctrl);
    p = lookup(player,&pos);
    expect(p->x == 2 && p->y == 1);
    struct glyph *g = lookup(enemy,&glyph);
    expect(g && g->ch == 'x');

    move(-3,0,w,h, constant_roll,&roll, &pos,&stats,&glyph,&ctrl);
    p = lookup(player,&pos);
    expect(p->x == 2 && p->y == 1);
}


test(draw_empty) {
    struct cell fb[1] = {{'q',COLOR_DEFAULT}};
    __attribute__((cleanup(reset)))
    struct component pos   = {.size = sizeof(struct pos)},
                     glyph = {.size = sizeof(struct glyph)},
                     disp  = {.size = sizeof(struct disposition)};
    attach(1,&pos,&(struct pos){0,0});
    detach(1,&pos);
    attach(1,&glyph,&(struct glyph){'@'});
    detach(1,&glyph);
    draw(fb,0,0,&pos,&glyph,&disp);
    expect(fb[0].ch == 'q' && fb[0].color == COLOR_DEFAULT);
}

test(draw_missing_glyph) {
    enum { W = 2, H = 2 };
    struct cell fb[W*H];
    __attribute__((cleanup(reset)))
    struct component pos   = {.size = sizeof(struct pos)},
                     glyph = {.size = sizeof(struct glyph)},
                     disp  = {.size = sizeof(struct disposition)};
    attach(1,&pos,&(struct pos){1,1});
    draw(fb,W,H,&pos,&glyph,&disp);
    for (int i = 0; i < W*H; i++) {
        expect(fb[i].ch == '.' && fb[i].color == COLOR_DEFAULT);
    }
}

test(draw_branches) {
    enum { W = 2, H = 2 };
    struct cell fb[W*H];
    __attribute__((cleanup(reset)))
    struct component pos   = {.size = sizeof(struct pos)},
                     glyph = {.size = sizeof(struct glyph)},
                     disp  = {.size = sizeof(struct disposition)};
    attach(1,&pos,&(struct pos){-1,0});
    attach(1,&glyph,&(struct glyph){'a'});
    attach(2,&pos,&(struct pos){0,-1});
    attach(2,&glyph,&(struct glyph){'b'});
    attach(3,&pos,&(struct pos){0,H});
    attach(3,&glyph,&(struct glyph){'c'});
    attach(4,&pos,&(struct pos){0,0});
    attach(4,&glyph,&(struct glyph){'d'});
    draw(fb,W,H,&pos,&glyph,&disp);
    expect(fb[0].ch == 'd' && fb[0].color == COLOR_DEFAULT);
}

test(draw_colors) {
    enum { W = 6, H = 1 };
    struct cell fb[W*H];
    __attribute__((cleanup(reset)))
    struct component pos   = {.size = sizeof(struct pos)},
                     glyph = {.size = sizeof(struct glyph)},
                     disp  = {.size = sizeof(struct disposition)};

    attach(1,&pos,&(struct pos){0,0});
    attach(1,&glyph,&(struct glyph){'a'});
    attach(1,&disp,&(struct disposition){DISPOSITION_IN_PARTY});

    attach(2,&pos,&(struct pos){1,0});
    attach(2,&glyph,&(struct glyph){'b'});
    attach(2,&disp,&(struct disposition){DISPOSITION_FRIENDLY});

    attach(3,&pos,&(struct pos){2,0});
    attach(3,&glyph,&(struct glyph){'c'});
    attach(3,&disp,&(struct disposition){DISPOSITION_NEUTRAL});

    attach(4,&pos,&(struct pos){3,0});
    attach(4,&glyph,&(struct glyph){'d'});
    attach(4,&disp,&(struct disposition){DISPOSITION_HOSTILE});

    attach(5,&pos,&(struct pos){4,0});
    attach(5,&glyph,&(struct glyph){'e'});
    attach(5,&disp,&(struct disposition){DISPOSITION_MADDENED});

    attach(6,&pos,&(struct pos){5,0});
    attach(6,&glyph,&(struct glyph){'f'});
    attach(6,&disp,&(struct disposition){(enum disposition_kind)99});

    draw(fb,W,H,&pos,&glyph,&disp);

    expect(fb[0].color == COLOR_GREEN);
    expect(fb[1].color == COLOR_BLUE);
    expect(fb[2].color == COLOR_YELLOW);
    expect(fb[3].color == COLOR_RED);
    expect(fb[4].color == COLOR_PURPLE);
    expect(fb[5].color == COLOR_DEFAULT);
}

test(entity_at_empty) {
    __attribute__((cleanup(reset)))
    struct component pos = {.size = sizeof(struct pos)};
    attach(1,&pos,&(struct pos){0,0});
    detach(1,&pos);
    expect(entity_at(0,0,&pos) == 0);
}

test(entity_at_same_x) {
    __attribute__((cleanup(reset)))
    struct component pos = {.size = sizeof(struct pos)};
    attach(1,&pos,&(struct pos){1,1});
    attach(2,&pos,&(struct pos){1,2});
    expect(entity_at(1,0,&pos) == 0);
    expect(entity_at(1,2,&pos) == 2);
}

test(alive_empty) {
    __attribute__((cleanup(reset)))
    struct component stats = {.size = sizeof(struct stats)},
                     disp  = {.size = sizeof(struct disposition)};
    attach(1,&stats,&(struct stats){0});
    attach(1,&disp, &(struct disposition){DISPOSITION_IN_PARTY});
    detach(1,&stats);
    detach(1,&disp);
    expect(!alive(&stats,&disp));
}

test(alive_missing_stats) {
    __attribute__((cleanup(reset)))
    struct component stats = {.size = sizeof(struct stats)},
                     disp  = {.size = sizeof(struct disposition)};
    attach(1,&disp, &(struct disposition){DISPOSITION_IN_PARTY});
    expect(!alive(&stats,&disp));
}

test(alive_not_party) {
    __attribute__((cleanup(reset)))
    struct component stats = {.size = sizeof(struct stats)},
                     disp  = {.size = sizeof(struct disposition)};
    attach(1,&stats,&(struct stats){.hp=5});
    attach(1,&disp, &(struct disposition){DISPOSITION_FRIENDLY});
    expect(!alive(&stats,&disp));
}

test(combat_miss) {
    __attribute__((cleanup(reset)))
    struct component stats = {.size = sizeof(struct stats)},
                     glyph = {.size = sizeof(struct glyph)},
                     ctrl  = {0};
    int a = 1, d = 2;
    attach(a,&stats,&(struct stats){.hp=5,.ac=10,.atk=0,.dmg=1});
    attach(d,&stats,&(struct stats){.hp=5,.ac=22,.atk=0,.dmg=0});
    int roll = 10;
    combat(a,d, constant_roll,&roll, &stats,&glyph,&ctrl);
    struct stats *s = lookup(d,&stats);
    expect(s && s->hp == 5);
}

test(combat_damage) {
    __attribute__((cleanup(reset)))
    struct component stats = {.size = sizeof(struct stats)},
                     glyph = {.size = sizeof(struct glyph)},
                     ctrl  = {0};
    int a = 1, d = 2;
    attach(a,&stats,&(struct stats){.hp=5,.ac=10,.atk=2,.dmg=3});
    attach(d,&stats,&(struct stats){.hp=10,.ac=0,.atk=0,.dmg=0});
    int roll = 10;
    combat(a,d, constant_roll,&roll, &stats,&glyph,&ctrl);
    struct stats *s = lookup(d,&stats);
    expect(s && s->hp == 7);
}

test(combat_missing) {
    __attribute__((cleanup(reset)))
    struct component stats = {.size = sizeof(struct stats)},
                     glyph = {.size = sizeof(struct glyph)},
                     ctrl  = {0};
    attach(2,&stats,&(struct stats){.hp=5,.ac=0,.atk=0,.dmg=0});
    int roll = 10;
    combat(1,2, constant_roll,&roll, &stats,&glyph,&ctrl);
    struct stats *s = lookup(2,&stats);
    expect(s && s->hp == 5);
    combat(1,3, constant_roll,&roll, &stats,&glyph,&ctrl);
    expect(!lookup(3,&stats));
}

test(combat_no_defender) {
    __attribute__((cleanup(reset)))
    struct component stats = {.size = sizeof(struct stats)},
                     glyph = {.size = sizeof(struct glyph)},
                     ctrl  = {0};
    attach(1,&stats,&(struct stats){.hp=5,.ac=0,.atk=2,.dmg=2});
    int roll = 10;
    combat(1,2, constant_roll,&roll, &stats,&glyph,&ctrl);
    expect(!lookup(2,&glyph));
}

test(move_none) {
    __attribute__((cleanup(reset)))
    struct component pos   = {.size = sizeof(struct pos)},
                     stats = {.size = sizeof(struct stats)},
                     glyph = {.size = sizeof(struct glyph)},
                     ctrl  = {0};
    attach(1,&ctrl,NULL);
    detach(1,&ctrl);
    int roll = 10;
    move(1,1,5,5, constant_roll,&roll, &pos,&stats,&glyph,&ctrl);
}

test(move_bounds) {
    int const w = 2, h = 2;
    __attribute__((cleanup(reset)))
    struct component pos   = {.size = sizeof(struct pos)},
                     stats = {.size = sizeof(struct stats)},
                     glyph = {.size = sizeof(struct glyph)},
                     ctrl  = {0};
    int id = 1;
    attach(id,&pos,&(struct pos){0,0});
    attach(id,&ctrl,NULL);

    int roll = 10;
    move(0,-1,w,h, constant_roll,&roll, &pos,&stats,&glyph,&ctrl); /* y<0 */
    struct pos *p = lookup(id,&pos);
    expect(p->x == 0 && p->y == 0);

    p->x = w-1; p->y = 0;
    move(1,0,w,h, constant_roll,&roll, &pos,&stats,&glyph,&ctrl); /* x>=w */
    expect(p->x == w-1 && p->y == 0);

    p->x = 0; p->y = h-1;
    move(0,1,w,h, constant_roll,&roll, &pos,&stats,&glyph,&ctrl); /* y>=h */
    expect(p->x == 0 && p->y == h-1);
}

test(combat_crit_success) {
    __attribute__((cleanup(reset)))
    struct component stats = {.size = sizeof(struct stats)},
                     glyph = {.size = sizeof(struct glyph)},
                     ctrl  = {0};
    int a = 1, d = 2;
    attach(a,&stats,&(struct stats){.hp=5,.ac=0,.atk=0,.dmg=2});
    attach(d,&stats,&(struct stats){.hp=5,.ac=100,.atk=0,.dmg=0});
    int roll = 20;
    combat(a,d, constant_roll,&roll, &stats,&glyph,&ctrl);
    struct stats *s = lookup(d,&stats);
    expect(s && s->hp == 3);
}

test(combat_crit_fail) {
    __attribute__((cleanup(reset)))
    struct component stats = {.size = sizeof(struct stats)},
                     glyph = {.size = sizeof(struct glyph)},
                     ctrl  = {0};
    int a = 1, d = 2;
    attach(a,&stats,&(struct stats){.hp=5,.ac=0,.atk=100,.dmg=2});
    attach(d,&stats,&(struct stats){.hp=5,.ac=0,.atk=0,.dmg=0});
    int roll = 1;
    combat(a,d, constant_roll,&roll, &stats,&glyph,&ctrl);
    struct stats *s = lookup(d,&stats);
    expect(s && s->hp == 5);
}
