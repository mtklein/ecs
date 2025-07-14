#define _POSIX_C_SOURCE 200809L
#include "ecs.c"
#include "pma.c"
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

static double pma_dense(int n) {
    __attribute__((cleanup(pma_reset)))
    struct pma p = {.size = sizeof(int)};
    double const start = now();
    for (int i = 0; i < n; i++) {
        pma_attach(i,&p,&i);
    }
    return now() - start;
}

static double pma_dense_rev(int n) {
    __attribute__((cleanup(pma_reset)))
    struct pma p = {.size = sizeof(int)};
    double const start = now();
    for (int i = n-1; i >= 0; i--) {
        pma_attach(i,&p,&i);
    }
    return now() - start;
}

static double pma_sparse(int n) {
    __attribute__((cleanup(pma_reset)))
    struct pma p = {.size = sizeof(int)};
    double const start = now();
    for (int i = 0; i < n; i++) {
        int id = i*10;
        pma_attach(id,&p,&id);
    }
    return now() - start;
}

static double pma_iter(int n) {
    __attribute__((cleanup(pma_reset)))
    struct pma p = {.size = sizeof(int)};
    for (int i = 0; i < n; i++) {
        pma_attach(i,&p,&i);
    }
    double const start = now();
    int const *val = p.data;
    int sum = 0;
    for (int i = 0; i < p.n; i++) {
        sum += val[i];
    }
    volatile int sink = 0;
    sink += sum;
    return now() - start;
}

static double pma_lookup_bench(int n) {
    __attribute__((cleanup(pma_reset)))
    struct pma p = {.size = sizeof(int)};
    for (int i = 0; i < n; i++) {
        pma_attach(i,&p,&i);
    }
    double const start = now();
    int sum = 0;
    for (int i = 0; i < p.n; i++) {
        int const *val = pma_lookup(p.id[i],&p);
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
    bench(pattern, "pma_dense",     pma_dense);
    bench(pattern, "pma_dense_rev", pma_dense_rev);
    bench(pattern, "pma_sparse",    pma_sparse);
    bench(pattern, "pma_iter",      pma_iter);
    bench(pattern, "pma_lookup",    pma_lookup_bench);
    return 0;
}
