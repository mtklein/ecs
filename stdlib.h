#pragma once

#include <stdlib.h>
#include <string.h>

static inline void* realloc_(void *ptr, size_t len) {
    if (!len) {
        free(ptr);
        return NULL;
    }
    return realloc(ptr, len);
}
#if defined(realloc)
    #undef  realloc
#endif
#define realloc realloc_

static inline void* memcpy_(void *dst, void const *src, size_t len) {
    return len ? memcpy(dst,src,len) : dst;
}
#if defined(memcpy)
    #undef  memcpy
#endif
#define memcpy memcpy_
