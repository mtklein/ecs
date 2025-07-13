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

    __attribute__((cleanup(reset)))
    struct component
        pos   = {.size=sizeof(struct pos)},
        stats = {.size=sizeof(struct stats)},
        glyph = {.size=sizeof(struct glyph)},
        controlled={0}, in_party={0},
        friendly={0}, hostile={0}, maddened={0};

    {
        int const player = next_id++;
        attach(player, &pos  , &(struct pos){1,1});
        attach(player, &stats, &(struct stats){.hp=10, .ac=10, .atk=2, .dmg=4});
        attach(player, &glyph, &(struct glyph){'@'});
        attach(player, &controlled, NULL);
        attach(player, &in_party  , NULL);
    }

    {
        int const imp = next_id++;
        attach(imp, &pos  , &(struct pos){3,1});
        attach(imp, &stats, &(struct stats){.hp=4, .ac=12, .atk=3, .dmg=2});
        attach(imp, &glyph, &(struct glyph){'i'});
        attach(imp, &hostile, NULL);
    }

    int const w=10, h=5;
    char *fb = calloc((size_t)(w*h), sizeof *fb);
    enum color *cb = calloc((size_t)(w*h), sizeof *cb);

    while (alive(&stats, &in_party)) {
        draw_disposition(fb,cb,w,h,
                        &pos,&glyph,
                        &in_party,&friendly,&hostile,&maddened);

        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                enum color const c = cb[y*w + x];
                char const *ansi = "";
                switch (c) {
                    case COLOR_NONE:        break;
                    case COLOR_GREEN:       ansi = "\033[32m"; break;
                    case COLOR_BLUE:        ansi = "\033[34m"; break;
                    case COLOR_DARK_YELLOW: ansi = "\033[33m"; break;
                    case COLOR_RED:         ansi = "\033[31m"; break;
                    case COLOR_PURPLE:      ansi = "\033[35m"; break;
                }
                fputs(ansi, stdout);
                putchar(fb[y*w + x]);
                if (*ansi) {
                    fputs("\033[0m", stdout);
                }
            }
            putchar('\n');
        }

        switch (getch()) {
            case 'q': return 0;

            case 'h': move(-1,0,w,h, d20,&seed, &pos,&stats,&glyph,&controlled); break;
            case 'j': move(0,+1,w,h, d20,&seed, &pos,&stats,&glyph,&controlled); break;
            case 'k': move(0,-1,w,h, d20,&seed, &pos,&stats,&glyph,&controlled); break;
            case 'l': move(+1,0,w,h, d20,&seed, &pos,&stats,&glyph,&controlled); break;
        }
        printf("\033[%dA",h);
    }
    return 0;
}
