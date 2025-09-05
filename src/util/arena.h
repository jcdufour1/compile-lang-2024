#ifndef ARENA_H
#define ARENA_H

#include <stddef.h>
#include <stdbool.h>
#include <assert.h>
#include "util.h"

typedef struct Arena_buf_ {
    size_t count; // count of bytes used including the Arena node itself
    size_t capacity; // total count of bytes alloced in this node including the Arena tast itself
    struct Arena_buf_* next;
} Arena_buf;

typedef struct {
    Arena_buf* next;
} Arena;

extern Arena a_main;
extern Arena a_print;

// allocate a zero-initialized memory region in arena, return pointer (similer to malloc, but never return null)
void* arena_alloc(Arena* arena, size_t capacity);

// zero-initialized
void* arena_realloc(Arena* arena, void* old_buf, size_t old_capacity, size_t new_capacity);

// reset, but do not free, allocated area
void arena_reset(Arena* arena);

void arena_free_internal(Arena* arena);

#define arena_free(arena) \
    do { \
        arena_free_internal(arena); \
        (arena)->next = NULL; \
    } while (0);

size_t arena_get_total_capacity(const Arena* arena);

size_t arena_get_total_usage(const Arena* arena);

// will return null string (by allocating one byte more than count)
static inline const char* arena_strndup(Arena* arena, const char* cstr, size_t count) {
    if (!cstr) {
        return NULL;
    }
    char* new_cstr = arena_alloc(arena, count + 1);
    memcpy(new_cstr, cstr, count);
    assert(new_cstr[count] == 0);
    return new_cstr;
}

// will return null string
static inline char* arena_strndup_mut(Arena* arena, const char* cstr, size_t count) {
    char* new_cstr = arena_alloc(arena, count);
    memcpy(new_cstr, cstr, count);
    return new_cstr;
}

// will return null string
static inline const char* arena_strdup(Arena* arena, const char* cstr) {
    return arena_strndup(arena, cstr, strlen(cstr));
}

// will return null string
static inline char* arena_strdup_mut(Arena* arena, const char* cstr) {
    if (!cstr) {
        return NULL;
    }
    return arena_strndup_mut(arena, cstr, strlen(cstr) + 1);
}

#endif // ARENA_H
