#pragma once

#include <string.h>

static inline void* memcpy_(void *dst, void const *src, size_t len) {
    return len ? memcpy(dst,src,len) : dst;
}
#if defined(memcpy)
    #undef  memcpy
#endif
#define memcpy memcpy_
