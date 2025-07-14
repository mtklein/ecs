#include "ecs.c"
#include "tree.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static double now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

static double bench_dense(int n) {
    __attribute__((cleanup(reset)))
    struct component c = {.size = sizeof(int)};
    double const start = now();
    for (int i = 0; i < n; ++i) {
        attach(i, &c, &i);
    }
    double const elapsed = now() - start;
    return elapsed;
}

static double bench_dense_rev(int n) {
    __attribute__((cleanup(reset)))
    struct component c = {.size = sizeof(int)};
    double const start = now();
    for (int i = n - 1; i >= 0; --i) {
        attach(i, &c, &i);
    }
    double const elapsed = now() - start;
    return elapsed;
}

static double bench_sparse(int n) {
    __attribute__((cleanup(reset)))
    struct component c = {.size = sizeof(int)};
    double const start = now();
    for (int i = 0; i < n; ++i) {
        int id = i * 10;
        attach(id, &c, &id);
    }
    double const elapsed = now() - start;
    return elapsed;
}

static double bench_iter(int n) {
    __attribute__((cleanup(reset)))
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
    volatile int sink = 0;
    sink += sum;
    double const elapsed = now() - start;
    return elapsed;
}
static double bench_lookup(int n) {
    __attribute__((cleanup(reset)))
    struct component c = {.size = sizeof(int)};
    for (int i = 0; i < n; i++) {
        attach(i, &c, &i);
    }
    double const start = now();
    int sum = 0;
    for (int const *id = c.id; id < c.id + c.n; id++) {
        int const *val = lookup(*id, &c);
        sum += *val;
    }
    volatile int sink = 0;
    sink += sum;
    double const elapsed = now() - start;
    return elapsed;
}

static double bench_tree_dense(int n) {
    __attribute__((cleanup(tree_reset)))
    struct tree t = {.size = sizeof(int)};
    double const start = now();
    for (int i = 0; i < n; ++i) {
        tree_attach(i,&t,&i);
    }
    return now() - start;
}

static double bench_tree_lookup(int n) {
    __attribute__((cleanup(tree_reset)))
    struct tree t = {.size = sizeof(int)};
    for (int i = 0; i < n; i++) {
        tree_attach(i,&t,&i);
    }
    double const start = now();
    int sum = 0;
    for (int i = 0; i < n; i++) {
        int const *val = tree_lookup(i,&t);
        sum += *val;
    }
    volatile int sink = 0;
    sink += sum;
    return now() - start;
}

static void bench(char const *pattern,
                  char const *name,
                  double (*fn)(int)) {
    if (strstr(name, pattern)) {
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
}

int main(int argc, char const* argv[]) {
    char const *pattern = argc > 1 ? argv[1] : "";
    bench(pattern, "dense",     bench_dense);
    bench(pattern, "dense_rev", bench_dense_rev);
    bench(pattern, "sparse",    bench_sparse);
    bench(pattern, "iter",      bench_iter);
    bench(pattern, "lookup",    bench_lookup);
    bench(pattern, "tree_dense",  bench_tree_dense);
    bench(pattern, "tree_lookup", bench_tree_lookup);
    return 0;
}
