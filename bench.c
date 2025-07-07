#include "ecs.h"
#include <math.h>
#include <stdio.h>
#include <time.h>

#define len(x) (int)(sizeof x / sizeof *x)
static volatile int sink;

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
    table_drop(&t);
    return elapsed;
}

static double bench_dense_rev(int n) {
    struct table t = {.size = sizeof(int)};
    double const start = now();
    for (int i = n - 1; i >= 0; --i) {
        table_set(&t, i, &i);
    }
    double const elapsed = now() - start;
    table_drop(&t);
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
    table_drop(&t);
    return elapsed;
}

static double bench_iter_direct(int n) {
    struct table t = {.size = sizeof(int)};
    for (int i = 0; i < n; i++) {
        table_set(&t, i, &i);
    }
    double const start = now();
    int const *val = t.data;
    int sum = 0;
    for (int i = 0; i < t.n; i++) {
        sum += val[i];
    }
    sink += sum;
    double const elapsed = now() - start;
    table_drop(&t);
    return elapsed;
}

static double bench_join_single(int n) {
    struct table t = {.size = sizeof(int)};
    for (int i = 0; i < n; i++) {
        table_set(&t, i, &i);
    }
    struct table *table[] = {&t};
    int key = ~0, val, sum = 0;
    double const start = now();
    while (table_join(table, len(table), &key, &val)) {
        sum += val;
    }
    sink += sum;
    double const elapsed = now() - start;
    table_drop(&t);
    return elapsed;
}

static double bench_join_small_large(int n) {
    int const small_n = n / 2;
    struct table small = {.size = sizeof(int)};
    struct table large = {.size = sizeof(int)};
    for (int i = 0; i < small_n; i++) {
        table_set(&small, i, &i);
        table_set(&large, i, &i);
    }
    for (int i = small_n; i < n; i++) {
        table_set(&large, i, &i);
    }
    struct table *table[] = {&small,&large};
    int key = ~0, vals[2], sum = 0;
    double const start = now();
    while (table_join(table, len(table), &key, vals)) {
        sum += vals[0] + vals[1];
    }
    sink += sum;
    double const elapsed = now() - start;
    table_drop(&small);
    table_drop(&large);
    return elapsed;
}

static double bench_join_large_small(int n) {
    int const small_n = n / 2;
    struct table small = {.size = sizeof(int)};
    struct table large = {.size = sizeof(int)};
    for (int i = 0; i < small_n; i++) {
        table_set(&small, i, &i);
        table_set(&large, i, &i);
    }
    for (int i = small_n; i < n; i++) {
        table_set(&large, i, &i);
    }
    struct table *table[] = {&large,&small};
    int key = ~0, vals[2], sum = 0;
    double const start = now();
    while (table_join(table, len(table), &key, vals)) {
        sum += vals[0] + vals[1];
    }
    sink += sum;
    double const elapsed = now() - start;
    table_drop(&small);
    table_drop(&large);
    return elapsed;
}

static void run(char const *name, double (*fn)(int)) {
    int const samples = 4;
    printf("%s\n", name);
    printf("%10s %9s\n", "n", "ns/n");
    double min = 0;
    for (int n = 1024; min < 0.125 / samples; n *= 2) {
        min = 1/0.0;
        double max = -1/0.0;
        for (int i = 0; i < samples; i++) {
            double const t = fn(n);
            if (min > t) { min = t; }
            if (max < t) { max = t; }
        }
        printf("%10d %4.1f–%4.1f ", n, min*1e9/n, max*1e9/n);
        long i = 0;
        for (; i < lrint(min*1e9/n); i++) { printf("█"); }
        for (; i < lrint(max*1e9/n); i++) { printf("⬚"); }
        printf("\n");
    }
    printf("\n");
}

int main(void) {
    run("dense",     bench_dense);
    run("dense_rev", bench_dense_rev);
    run("sparse",    bench_sparse);
    run("iter1",     bench_iter_direct);
    run("join1",     bench_join_single);
    run("join_sl",   bench_join_small_large);
    run("join_ls",   bench_join_large_small);
    return 0;
}
