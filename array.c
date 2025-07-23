#include "array.h"
#include <stdlib.h>

int grow(array *arr) {
    if (arr->cap == arr->n) {
        arr->cap  = arr->cap ? 2*arr->cap : 1;
        arr->data = realloc(arr->data, (size_t)arr->cap * arr->size);
    }
    return arr->n++;
}

void* ptr(array const *arr, int ix) {
    return (char*)arr->data + (size_t)ix * arr->size;
}

void* pop(array *arr) {
    return arr->n ? ptr(arr, --arr->n) : NULL;
}
