#include "ecs.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
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
    run("iter1",     bench_iter_direct);
    run("join1",     bench_join_single);
    run("join_sl",   bench_join_small_large);
    run("join_ls",   bench_join_large_small);
    return 0;
}
