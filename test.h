#pragma once

void expect_fail(char const *expr, char const *file, int line);
#define expect(x) if (!(x)) expect_fail(#x, __FILE__, __LINE__)

#define test(name) __attribute__((constructor(102))) static void test_##name(void)
