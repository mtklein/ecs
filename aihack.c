#include "ecs.h"
#include "systems.h"
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

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

__attribute__((constructor))
static void premain(void) {
    setenv("LLVM_PROFILE_FILE", "%t/tmp.profraw", 0);
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

    int next_id = 1;

    struct component c[] = {
        [POS]   = {.size=sizeof(struct pos)},
        [STATS] = {.size=sizeof(struct stats)},
        [GLYPH] = {.size=sizeof(struct glyph)},
        [DISP]  = {.size=sizeof(enum disposition)},
        [IS_CONTROLLED] = {0},
    };

    {
        int const player = next_id++;
        attach(player, c+POS  , &(struct pos){1,1});
        attach(player, c+STATS, &(struct stats){.hp=10, .ac=10, .atk=2, .dmg=4});
        attach(player, c+GLYPH, &(struct glyph){'@'});
        attach(player, c+DISP , &(enum disposition){PARTY});
        attach(player, c+IS_CONTROLLED, NULL);
    }

    {
        int const imp = next_id++;
        attach(imp, c+POS  , &(struct pos){3,1});
        attach(imp, c+STATS, &(struct stats){.hp=4, .ac=12, .atk=3, .dmg=2});
        attach(imp, c+GLYPH, &(struct glyph){'i'});
        attach(imp, c+DISP , &(enum disposition){HOSTILE});
    }

    int const w=10, h=5;
    struct pixel *fb = calloc((size_t)(w*h), sizeof *fb);

    while (alive(c)) {
        draw(fb,w,h,c);

        static char const *color[] = {
            [INERT]    = "\033[0m",
            [PARTY]    = "\033[32m",
            [FRIENDLY] = "\033[34m",
            [NEUTRAL]  = "\033[33m",
            [HOSTILE]  = "\033[31m",
            [MADDENED] = "\033[35m",
        };
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                struct pixel px = fb[y*w + x];
                printf("%s%c", color[px.disp], px.glyph.ch);
            }
            putchar('\n');
        }

        switch (getch()) {
            case 'q': return 0;

            case 'h': move(-1,0,w,h, d20,&seed,c); break;
            case 'j': move(0,+1,w,h, d20,&seed,c); break;
            case 'k': move(0,-1,w,h, d20,&seed,c); break;
            case 'l': move(+1,0,w,h, d20,&seed,c); break;
        }
        printf("\033[%dA",h);
    }
    return 0;
}
