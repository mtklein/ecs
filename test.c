#define _POSIX_C_SOURCE 200809L
#include "test.h"
#include <stdio.h>
#include <stdlib.h>

void expect_fail(char const *expr, char const *file, int line) {
    fprintf(stderr, "%s:%d expect(%s)\n", file, line, expr);
    __builtin_debugtrap();
}

__attribute__((constructor(101)))
static void premain(void) {
    setenv("LLVM_PROFILE_FILE", "%t/tmp.profraw", 0);
}

int main(void) {
    return 0;
}
