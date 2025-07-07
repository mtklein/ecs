#include "ecs.h"
#include <math.h>
#include <stdio.h>
#include <time.h>

static double now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

static double bench_dense(int n) {
    struct table t = {.size = sizeof(int)};
    double const start = now();
    for (int i = 0; i < n; ++i) {
        table_set(&t, i, &i);
    }
    double const elapsed = now() - start;
    table_reset(&t);
    return elapsed;
}

static double bench_dense_rev(int n) {
    struct table t = {.size = sizeof(int)};
    double const start = now();
    for (int i = n - 1; i >= 0; --i) {
        table_set(&t, i, &i);
    }
    double const elapsed = now() - start;
    table_reset(&t);
    return elapsed;
}

static double bench_sparse(int n) {
    struct table t = {.size = sizeof(int)};
    double const start = now();
    for (int i = 0; i < n; ++i) {
        int key = i * 10;
        table_set(&t, key, &key);
    }
    double const elapsed = now() - start;
    table_reset(&t);
    return elapsed;
}

static void run(char const *name, double (*fn)(int)) {
    printf("%s\n", name);
    printf("%8s  %8s %8s\n", "n", "µs", "ns/n");
    double t = 0;
    for (int n = 1024; t < 0.125; n *= 2) {
        t = fn(n);
        printf("%8d %8.0f %8.0f ", n, t*1e6, t*1e9/n);
        for (int i = 0; i < (int)(t*1e9/n); i++) {
            printf("█");
        }
        printf("\n");
    }
    printf("\n");
}

int main(void) {
    run("dense",     bench_dense);
    run("dense_rev", bench_dense_rev);
    run("sparse",    bench_sparse);
    return 0;
}
