#include "array.h"
#include <stdio.h>
#include <stdlib.h>

#ifndef __has_builtin
#   define __has_builtin(x) 0
#endif
#if !__has_builtin(__builtin_debugtrap)
#   define __builtin_debugtrap() __builtin_trap()
#endif
#define expect(x) \
    if (!(x)) fprintf(stderr, "%s:%d expect(%s)\n", __FILE__, __LINE__, #x), __builtin_debugtrap()

static void free_data(void *p) {
    array *arr = p;
    free(arr->data);
    arr->data = NULL;
}

static void test_push_pop(void) {
    __attribute__((cleanup(free_data)))
    array arr = {.size = sizeof(int)};

    expect(push(&arr) == 0);
    expect(arr.n == 1);
    expect(arr.cap == 1);
    *(int *)ptr(&arr, 0) = 11;

    expect(push(&arr) == 1);
    expect(arr.n == 2);
    expect(arr.cap == 2);
    *(int *)ptr(&arr, 1) = 22;

    expect(push(&arr) == 2);
    expect(arr.n == 3);
    expect(arr.cap == 4);
    *(int *)ptr(&arr, 2) = 33;

    int *val = pop(&arr);
    expect(val && *val == 33);
    expect(arr.n == 2);

    val = pop(&arr);
    expect(val && *val == 22);
    expect(arr.n == 1);

    val = pop(&arr);
    expect(val && *val == 11);
    expect(arr.n == 0);

    expect(pop(&arr) == NULL);
}

int main(void) {
    test_push_pop();
    return 0;
}
