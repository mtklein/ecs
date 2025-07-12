#ifndef TEST_H
#define TEST_H
#include <stdio.h>

static inline void expect_(_Bool x, char const *expr,
                           char const *file, int line) {
    if (!x) { fprintf(stderr, "%s:%d expect(%s)\n", file, line, expr); __builtin_debugtrap(); }
}
#define expect(x) expect_(x, #x, __FILE__, __LINE__)

#endif // TEST_H
