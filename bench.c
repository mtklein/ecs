#include "ecs.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static unsigned rng(unsigned seed) {
    __builtin_mul_overflow(seed, 1103515245u, &seed);
    __builtin_add_overflow(seed,      12345u, &seed);
    return seed;
}

static double now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

static void free_component(void *p) {
    struct component *c = p;
    free(c->data);
    free(c->id);
    free(c->ix);
}

static double bench_ascending(int n) {
    __attribute__((cleanup(free_component))) component(int) comp = {0};

    double const start = now();
    for (int i = 0; i < n; i++) {
        component_attach(&comp, i);
    }
    return now() - start;
}

static double bench_descending(int n) {
    __attribute__((cleanup(free_component))) component(int) comp = {0};

    double const start = now();
    while (n --> 0) {
        component_attach(&comp, n);
    }
    return now() - start;
}

static double bench_sparse(int n) {
    __attribute__((cleanup(free_component))) component(int) comp = {0};

    double const start = now();
    for (int i = 0; i < n; i++) {
        component_attach(&comp, i*10);
    }
    return now() - start;
}

static double bench_random(int n) {
    __attribute__((cleanup(free_component))) component(int) comp = {0};
    unsigned seed = 1;

    double const start = now();
    for (int i = 0; i < n; i++) {
        seed = rng(seed);
        int const id = (int)(seed % (unsigned)n);
        component_attach(&comp, id);
    }
    return now() - start;
}

static void run(char const *pattern,
                char const *name,
                double (*fn)(int)) {
    if (strstr(name, pattern)) {
        printf("%s\n", name);

        double min = 0, max = 1;
        for (int lgN = 10; min < max; lgN++) {
            int const N = 1<<lgN;
            min = +1/0.0;
            max = -1/0.0;
            for (double const start = now(); now() - start < 1/16.0;) {
                double const t = fn(N);
                if (min > t) { min = t; }
                if (max < t) { max = t; }
            }
            printf("%s%2d %5.1f – %5.1f%s ", lgN==10 ? "N=2^" : "    "
                                           , lgN
                                           , min*1e9/N
                                           , max*1e9/N
                                           , lgN==10 ? "ns" : "  ");
            long i = 0;
            for (; i < lrint(min*1e9/N) && i < 80; i++) { printf("●"); }
            for (; i < lrint(max*1e9/N) && i < 80; i++) { printf("◌"); }
            printf("\n");
        }
        printf("\n");
    }
}

int main(int argc, char const* argv[]) {
    char const *pattern = argc > 1 ? argv[1] : "";
    run(pattern, "ascending",  bench_ascending);
    run(pattern, "descending", bench_descending);
    run(pattern, "sparse",     bench_sparse);
    run(pattern, "random",     bench_random);
    return 0;
}

