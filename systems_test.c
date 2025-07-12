#include "ecs.h"
#include "systems.h"
#include <stdlib.h>
#include <stdio.h>

static inline void expect_(_Bool x, const char *expr, const char *file, int line) {
    if (!x) { fprintf(stderr, "%s:%d expect(%s)\n", file, line, expr); __builtin_debugtrap(); }
}
#define expect(x) expect_(x, #x, __FILE__, __LINE__)

static void test_draw(void) {
    enum { W = 3, H = 2 };
    int const w = W, h = H;
    char fb[W*H];

    __attribute__((cleanup(reset)))
    struct component pos   = {.size = sizeof(struct pos)},
                     glyph = {.size = sizeof(struct glyph)};

    attach(1, &pos, &(struct pos){0,0});
    attach(1, &glyph, &(struct glyph){'a'});
    attach(2, &pos, &(struct pos){2,1});
    attach(2, &glyph, &(struct glyph){'b'});
    attach(3, &pos, &(struct pos){4,4});
    attach(3, &glyph, &(struct glyph){'c'});

    draw(fb,w,h,&pos,&glyph);

    expect(fb[0] == 'a');
    expect(fb[5] == 'b');
    for (int i = 0; i < w*h; i++) {
        if (i == 0 || i == 5) { continue; }
        expect(fb[i] == '.');
    }

}

static void test_entity_at(void) {
    __attribute__((cleanup(reset)))
    struct component pos = {.size = sizeof(struct pos)};
    attach(1, &pos, &(struct pos){1,0});
    attach(2, &pos, &(struct pos){2,1});

    expect(entity_at(1,0,&pos) == 1);
    expect(entity_at(2,1,&pos) == 2);
    expect(entity_at(0,0,&pos) == 0);

}

static void test_alive(void) {
    __attribute__((cleanup(reset)))
    struct component stats = {.size = sizeof(struct stats)},
                     party = {0};

    attach(1, &stats, &(struct stats){.hp=0});
    attach(1, &party, NULL);
    attach(2, &stats, &(struct stats){.hp=5});
    attach(2, &party, NULL);

    expect(alive(&stats,&party));

    struct stats *s = lookup(2, &stats);
    s->hp = 0;
    expect(!alive(&stats,&party));

}

static void test_kill(void) {
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

static void test_combat(void) {
    __attribute__((cleanup(reset)))
    struct component stats = {.size = sizeof(struct stats)},
                     glyph = {.size = sizeof(struct glyph)},
                     ctrl  = {0};

    int attacker = 1;
    int defender = 2;

    attach(attacker,&stats,&(struct stats){.hp=10,.ac=10,.atk=2,.dmg=4});
    attach(defender,&stats,&(struct stats){.hp=3,.ac=0,.atk=0,.dmg=0});
    attach(defender,&glyph,&(struct glyph){'e'});

    combat(attacker,defender,&stats,&glyph,&ctrl);

    expect(!lookup(defender,&stats));
    struct glyph *g = lookup(defender,&glyph);
    expect(g && g->ch == 'x');

}

static void test_move(void) {
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

    move(1,0,w,h,&pos,&stats,&glyph,&ctrl);
    struct pos *p = lookup(player,&pos);
    expect(p->x == 2 && p->y == 1);

    move(1,0,w,h,&pos,&stats,&glyph,&ctrl);
    p = lookup(player,&pos);
    expect(p->x == 2 && p->y == 1);
    struct glyph *g = lookup(enemy,&glyph);
    expect(g && g->ch == 'x');

    move(-3,0,w,h,&pos,&stats,&glyph,&ctrl);
    p = lookup(player,&pos);
    expect(p->x == 2 && p->y == 1);

}

int main(void) {
    test_draw();
    test_entity_at();
    test_alive();
    test_kill();
    test_combat();
    test_move();
    return 0;
}

__attribute__((constructor))
static void premain(void) {
    setenv("LLVM_PROFILE_FILE", "%t/tmp.profraw", 0);
}

