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

struct sum_ctx { int tables,sum; };

static void add(int key, void* const row[], void *ctx) {
    struct sum_ctx *s = ctx;
    (void)key;
    for (int i = 0; i < s->tables; ++i) {
        if (row[i]) {
            s->sum += *(int*)row[i];
        }
    }
}

static double bench_iter(int n) {
    struct table t = {.size = sizeof(int)};
    for (int i = 0; i < n; ++i) {
        table_set(&t, i, &i);
    }
    struct sum_ctx ctx = {.tables = 1};
    struct table const *table[] = {&t};
    double const start = now();
    table_join(table, 1, &ctx, add);
    double const elapsed = now() - start;
    table_reset(&t);
    return elapsed + ctx.sum*0;
}

static double bench_join2_equal(int n) {
    struct table a = {.size = sizeof(int)};
    struct table b = {.size = sizeof(int)};
    for (int i = 0; i < n; ++i) {
        table_set(&a, i, &i);
        table_set(&b, i, &i);
    }
    struct sum_ctx ctx = {.tables = 2};
    struct table const *table[] = {&a,&b};
    double const start = now();
    table_join(table, 2, &ctx, add);
    double const elapsed = now() - start;
    table_reset(&a);
    table_reset(&b);
    return elapsed + ctx.sum*0;
}

static double bench_join2_diff(int n) {
    struct table big   = {.size = sizeof(int)};
    struct table small = {.size = sizeof(int)};
    for (int i = 0; i < n; ++i) {
        table_set(&big, i, &i);
    }
    for (int i = 0; i < n; i += 4) {
        table_set(&small, i, &i);
    }
    struct sum_ctx ctx = {.tables = 2};
    struct table const *table[] = {&big,&small};
    double const start = now();
    table_join(table, 2, &ctx, add);
    double const elapsed = now() - start;
    table_reset(&big);
    table_reset(&small);
    return elapsed + ctx.sum*0;
}

static double bench_join3_equal(int n) {
    struct table a = {.size = sizeof(int)};
    struct table b = {.size = sizeof(int)};
    struct table c = {.size = sizeof(int)};
    for (int i = 0; i < n; ++i) {
        table_set(&a, i, &i);
        table_set(&b, i, &i);
        table_set(&c, i, &i);
    }
    struct sum_ctx ctx = {.tables = 3};
    struct table const *table[] = {&a,&b,&c};
    double const start = now();
    table_join(table, 3, &ctx, add);
    double const elapsed = now() - start;
    table_reset(&a);
    table_reset(&b);
    table_reset(&c);
    return elapsed + ctx.sum*0;
}

static double bench_join3_diff(int n) {
    struct table big  = {.size = sizeof(int)};
    struct table mid  = {.size = sizeof(int)};
    struct table tiny = {.size = sizeof(int)};
    for (int i = 0; i < n; ++i) {
        table_set(&big, i, &i);
    }
    for (int i = 0; i < n; i += 2) {
        table_set(&mid, i, &i);
    }
    for (int i = 0; i < n; i += 4) {
        table_set(&tiny, i, &i);
    }
    struct sum_ctx ctx = {.tables = 3};
    struct table const *table[] = {&big,&mid,&tiny};
    double const start = now();
    table_join(table, 3, &ctx, add);
    double const elapsed = now() - start;
    table_reset(&big);
    table_reset(&mid);
    table_reset(&tiny);
    return elapsed + ctx.sum*0;
}

static void run(char const *name, double (*fn)(int)) {
    printf("%s\n", name);
    printf("%8s  %8s\n", "n", "sec");
    for (int n = 1024; n <= (1<<24); n *= 2) {
        double const t = fn(n);
        printf("%8d  %8.6f\n", n, t);
    }
    printf("\n");
}

int main(void) {
    run("dense",     bench_dense);
    run("dense_rev", bench_dense_rev);
    run("sparse",    bench_sparse);
    run("iter",      bench_iter);
    run("join2",     bench_join2_equal);
    run("join2d",    bench_join2_diff);
    run("join3",     bench_join3_equal);
    run("join3d",    bench_join3_diff);
    return 0;
}
