#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <dlfcn.h>

// Override free to zero-fill to make use-after-free predictable
extern "C" void free(void *ptr) {
    static void (*real_free)(void*) = nullptr;
    if (!real_free) real_free = (void(*)(void*))dlsym(RTLD_NEXT, "free");
    if (ptr) {
        size_t *size_ptr = (size_t*)((char*)ptr - 8);
        size_t size = *size_ptr & ~7;
        memset(ptr, 0xAA, size - 8);
    }
    real_free(ptr);
}

extern "C" void *malloc(size_t size) {
    static void* (*real_malloc)(size_t) = nullptr;
    if (!real_malloc) real_malloc = (void*(*)(size_t))dlsym(RTLD_NEXT, "malloc");
    return real_malloc(size);
}
