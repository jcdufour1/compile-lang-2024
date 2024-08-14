#ifndef UTIL_H
#define UTIL_H

#include <stdlib.h>
#include <stdio.h>

#define todo() \
    do { \
        fprintf(stderr, "%s:%d:error: not implemented", __FILE__, __LINE__); \
        abort(); \
    } while (0);

static void* safe_realloc(void* old_ptr, size_t new_capacity) {
    void* new_ptr = realloc(old_ptr, new_capacity);
    if (!new_ptr) {
        todo();
    }
    return new_ptr;
}

static void* safe_malloc(size_t capacity) {
    void* new_ptr = malloc(capacity);
    if (!new_ptr) {
        todo();
    }
    memset(new_ptr, 0, capacity);
    return new_ptr;
}

#endif // UTIL_H
