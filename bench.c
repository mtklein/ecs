#include "ecs.h"
#include <stdio.h>
#include <time.h>

static volatile int sink;

static double now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

static void fill(struct table *t, int n) {
    for (int i = 0; i < n; i++) {
        table_set(t, i, &i);
    }
}

static void add2(int key, void const *a, void const *b, void *ctx) {
    int const *x = a;
    int const *y = b;
    int       *s = ctx;
    *s += *x + *y + key;
}

static void add3(int key, void const *const*vals, void *ctx) {
    int const *x = vals[0];
    int const *y = vals[1];
    int const *z = vals[2];
    int       *s = ctx;
    *s += *x + *y + *z + key;
}

static double bench_scan(int n) {
    struct table t = {.size = sizeof(int)};
    fill(&t, n);
    double const start = now();
    int sum = 0;
    for (int i = 0; i < t.n; i++) {
        int const *v = table_get(&t, i);
        sum += *v;
    }
    sink += sum;
    double const elapsed = now() - start;
    table_reset(&t);
    return elapsed;
}

static double bench_join2_eq(int n) {
    struct table a = {.size = sizeof(int)};
    struct table b = {.size = sizeof(int)};
    fill(&a, n);
    fill(&b, n);
    double const start = now();
    int sum = 0;
    table_join(&a, &b, add2, &sum);
    sink += sum;
    double const elapsed = now() - start;
    table_reset(&a);
    table_reset(&b);
    return elapsed;
}

static double bench_join2_sl(int n) {
    int const small_n = n / 16 ? n / 16 : 1;
    struct table small = {.size = sizeof(int)};
    struct table large = {.size = sizeof(int)};
    fill(&small, small_n);
    fill(&large, n);
    double const start = now();
    int sum = 0;
    table_join(&small, &large, add2, &sum);
    sink += sum;
    double const elapsed = now() - start;
    table_reset(&small);
    table_reset(&large);
    return elapsed;
}

static double bench_join3_eq(int n) {
    struct table a = {.size = sizeof(int)};
    struct table b = {.size = sizeof(int)};
    struct table c = {.size = sizeof(int)};
    fill(&a, n);
    fill(&b, n);
    fill(&c, n);
    struct table const *t[] = {&a, &b, &c};
    double const start = now();
    int sum = 0;
    table_join_many(t, 3, add3, &sum);
    sink += sum;
    double const elapsed = now() - start;
    table_reset(&a);
    table_reset(&b);
    table_reset(&c);
    return elapsed;
}

static double bench_join3_sl(int n) {
    int const small_n = n / 16 ? n / 16 : 1;
    struct table small = {.size = sizeof(int)};
    struct table mid   = {.size = sizeof(int)};
    struct table large = {.size = sizeof(int)};
    fill(&small, small_n);
    fill(&mid, n);
    fill(&large, n);
    struct table const *t[] = {&small, &mid, &large};
    double const start = now();
    int sum = 0;
    table_join_many(t, 3, add3, &sum);
    sink += sum;
    double const elapsed = now() - start;
    table_reset(&small);
    table_reset(&mid);
    table_reset(&large);
    return elapsed;
}

static void run(char const *name, double (*fn)(int)) {
    printf("%s\n", name);
    printf("%8s  %8s\n", "n", "sec");
    for (int n = 1024; n <= (1<<20); n *= 2) {
        double const t = fn(n);
        printf("%8d  %8.6f\n", n, t);
    }
    printf("\n");
}

int main(void) {
    run("scan",        bench_scan);
    run("join2_eq",    bench_join2_eq);
    run("join2_sl",    bench_join2_sl);
    run("join3_eq",    bench_join3_eq);
    run("join3_sl",    bench_join3_sl);
    return 0;
}

