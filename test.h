#pragma once
#include <stdio.h>

#define expect(x) \
    if (!(x)) fprintf(stderr, "%s:%d expect(%s)\n", __FILE__, __LINE__, #x), __builtin_debugtrap()
#define TODO(x) expect(!(x))
