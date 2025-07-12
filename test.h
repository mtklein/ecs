#pragma once
#include <stdio.h>

static inline void expect_(_Bool x, char const *expr, char const *file, int line) {
    if (!x) { fprintf(stderr, "%s:%d expect(%s)\n", file, line, expr); __builtin_debugtrap(); }
}
#define expect(x) expect_(x, #x, __FILE__, __LINE__)

#define test(name) __attribute__((constructor(102))) static void test_##name(void)
