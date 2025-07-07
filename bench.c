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

static double bench_iter_direct(int n) {
    struct table t = {.size = sizeof(int)};
    for (int i = 0; i < n; i++) {
        table_set(&t, i, &i);
    }
    double const start = now();
    int sum = 0;
    for (int i = 0; i < t.n; i++) {
        sum += ((int*)t.data)[i];
    }
    sink += sum;
    double const elapsed = now() - start;
    table_reset(&t);
    return elapsed;
}

static double bench_join_single(int n) {
    struct table t = {.size = sizeof(int)};
    for (int i = 0; i < n; i++) {
        table_set(&t, i, &i);
    }
    struct table const *tables[] = {&t};
    int key = ~0, val, sum = 0;
    double const start = now();
    while (table_join(tables, len(tables), &key, &val)) {
        sum += val;
    }
    sink += sum;
    double const elapsed = now() - start;
    table_reset(&t);
    return elapsed;
}

static double bench_join_equal(int n) {
    struct table a = {.size = sizeof(int)};
    struct table b = {.size = sizeof(int)};
    for (int i = 0; i < n; i++) {
        table_set(&a, i, &i);
        table_set(&b, i, &i);
    }
    struct table const *tables[] = {&a,&b};
    int key = ~0, vals[2], sum = 0;
    double const start = now();
    while (table_join(tables, len(tables), &key, vals)) {
        sum += vals[0] + vals[1];
    }
    sink += sum;
    double const elapsed = now() - start;
    table_reset(&a);
    table_reset(&b);
    return elapsed;
}

static double bench_join_small_lead(int n) {
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
    struct table const *tables[] = {&small,&large};
    int key = ~0, vals[2], sum = 0;
    double const start = now();
    while (table_join(tables, len(tables), &key, vals)) {
        sum += vals[0] + vals[1];
    }
    sink += sum;
    double const elapsed = now() - start;
    table_reset(&small);
    table_reset(&large);
    return elapsed;
}

static double bench_join_large_lead(int n) {
    int const small_n = n / 2;
    struct table large = {.size = sizeof(int)};
    struct table small = {.size = sizeof(int)};
    for (int i = 0; i < small_n; i++) {
        table_set(&large, i, &i);
        table_set(&small, i, &i);
    }
    for (int i = small_n; i < n; i++) {
        table_set(&large, i, &i);
    }
    struct table const *tables[] = {&large,&small};
    int key = ~0, vals[2], sum = 0;
    double const start = now();
    while (table_join(tables, len(tables), &key, vals)) {
        sum += vals[0] + vals[1];
    }
    sink += sum;
    double const elapsed = now() - start;
    table_reset(&large);
    table_reset(&small);
    return elapsed;
}

static void run(char const *name, double (*fn)(int)) {
    int const samples = 4;
    printf("%s\n", name);
    printf("%8s %5s\n", "n", "ns/n");
    double min = 0;
    for (int n = 1024; min < 0.125 / samples; n *= 2) {
        min = 1/0.0;
        double max = -1/0.0;
        for (int i = 0; i < samples; i++) {
            double const t = fn(n);
            if (min > t) { min = t; }
            if (max < t) { max = t; }
        }
        printf("%8d %2d–%2d ", n, (int)(min*1e9/n), (int)(max*1e9/n));
        int i = 0;
        for (; i < (int)(min*1e9/n); i++) { printf("█"); }
        for (; i < (int)(max*1e9/n); i++) { printf("⬚"); }
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
    run("join_eq",   bench_join_equal);
    run("join_sl",   bench_join_small_lead);
    run("join_ll",   bench_join_large_lead);
    return 0;
}
