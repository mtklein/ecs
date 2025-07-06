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
    struct component c = {.size = sizeof(int)};
    double start = now();
    for (int i = 0; i < n; ++i) {
        component_attach(&c, i, &i);
    }
    double elapsed = now() - start;
    component_free(&c);
    return elapsed;
}

static double bench_dense_rev(int n) {
    struct component c = {.size = sizeof(int)};
    double start = now();
    for (int i = n - 1; i >= 0; --i) {
        component_attach(&c, i, &i);
    }
    double elapsed = now() - start;
    component_free(&c);
    return elapsed;
}

static double bench_sparse(int n) {
    struct component c = {.size = sizeof(int)};
    double start = now();
    for (int i = 0; i < n; ++i) {
        int key = i * 10;
        component_attach(&c, key, &key);
    }
    double elapsed = now() - start;
    component_free(&c);
    return elapsed;
}

static void run(char const *name, double (*fn)(int)) {
    printf("%s\n", name);
    printf("%8s %12s %8s %8s\n", "n", "sec", "sec/n", "sec/nlogn");
    for (int n = 1024; n <= 32768/*131072*/; n *= 2) {
        double const t = fn(n),
                    On = t / (double)n,
                Onlogn = t / ((double)n * log((double)n));
        printf("%8d %12.6f %8.2e %8.2e\n", n, t, On, Onlogn);
    }
    printf("\n");
}

int main(void) {
    run("dense",     bench_dense);
    run("dense_rev", bench_dense_rev);
    run("sparse",    bench_sparse);
    return 0;
}
