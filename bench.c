#include "ecs.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static volatile int sink;

static double now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

static double bench_dense(int n) {
    struct component c = {.size = sizeof(int)};
    double const start = now();
    for (int i = 0; i < n; ++i) {
        attach(i, &c, &i);
    }
    double const elapsed = now() - start;
    reset(&c);
    return elapsed;
}

static double bench_dense_rev(int n) {
    struct component c = {.size = sizeof(int)};
    double const start = now();
    for (int i = n - 1; i >= 0; --i) {
        attach(i, &c, &i);
    }
    double const elapsed = now() - start;
    reset(&c);
    return elapsed;
}

static double bench_sparse(int n) {
    struct component c = {.size = sizeof(int)};
    double const start = now();
    for (int i = 0; i < n; ++i) {
        int id = i * 10;
        attach(id, &c, &id);
    }
    double const elapsed = now() - start;
    reset(&c);
    return elapsed;
}

static double bench_iter(int n) {
    struct component c = {.size = sizeof(int)};
    for (int i = 0; i < n; i++) {
        attach(i, &c, &i);
    }
    double const start = now();
    int const *val = c.data;
    int sum = 0;
    for (int i = 0; i < c.n; i++) {
        sum += val[i];
    }
    sink += sum;
    double const elapsed = now() - start;
    reset(&c);
    return elapsed;
}
static double bench_lookup(int n) {
    struct component c = {.size = sizeof(int)};
    for (int i = 0; i < n; i++) {
        attach(i, &c, &i);
    }
    double const start = now();
    int sum = 0;
    for (int id = 0; id < n; id++) {
        int const *val = lookup(id, &c);
        sum += *val;
    }
    sink += sum;
    double const elapsed = now() - start;
    reset(&c);
    return elapsed;
}

static double bench_begin_end(int n) {
    struct component c = {.size = sizeof(int)};
    for (int i = 0; i < n; i++) {
        attach(i, &c, &i);
    }
    double const start = now();
    int sum = 0;
    for (int const *id = iter(&c), *end = stop(&c); id != end; id++) {
        int const *val = lookup(*id, &c);
        sum += *val;
    }
    sink += sum;
    double const elapsed = now() - start;
    reset(&c);
    return elapsed;
}

static char const *pattern = "";

static void run(char const *name, double (*fn)(int)) {
    if (!strstr(name, pattern)) {
        return;
    }
    printf("%s\n", name);

    int lgN = 10;
    double min,max;
    do {
        int const N = 1<<lgN;
        min = +1/0.0;
        max = -1/0.0;
        for (double const start = now(); now() - start < 1/16.0;) {
            double const t = fn(N);
            if (min > t) { min = t; }
            if (max < t) { max = t; }
        }
        printf("%s%2d %4.1f–%4.1f%s ", lgN==10 ? "N=2^" : "    ", lgN
                                     , min*1e9/N, max*1e9/N
                                     , lgN==10 ? "ns" : "  ");
        long i = 0;
        for (; i < lrint(min*1e9/N) && i < 80; i++) { printf("■"); }
        for (; i < lrint(max*1e9/N) && i < 80; i++) { printf("⬚"); }
        printf("\n");
        lgN++;
    } while (min < max);
    printf("\n");
}

int main(int argc, char const* argv[]) {
    if (argc > 1) { pattern = argv[1]; }
    run("dense",     bench_dense);
    run("dense_rev", bench_dense_rev);
    run("sparse",    bench_sparse);
    run("iter",      bench_iter);
    run("lookup",    bench_lookup);
    run("begin_end", bench_begin_end);
    return 0;
}
