#pragma once
#include <stdio.h>

#define expect(x) \
    if (!(x)) fprintf(stderr, "%s:%d expect(%s)\n", __FILE__, __LINE__, #x), __builtin_debugtrap()
#define TODO(x) expect(!(x))

static inline _Bool equiv(float x, float y) {
    return (x <= y && y <= x)
        || (x != x && y != y);
}
