#ifndef ARENA_H
#define ARENA_H

#include <stddef.h>
#include <stdbool.h>
#include "util.h"

typedef struct Arena_buf_ {
    size_t count; // count of bytes used including the Arena node itself
    size_t capacity; // total count of bytes alloced in this node including the Arena node itself
    struct Arena_buf_* next;
} Arena_buf;

typedef struct {
    Arena_buf* next;
} Arena;

extern Arena a_main;
extern Arena print_arena;

// allocate a zero-initialized memory region in arena, return pointer (similer to malloc, but never return null)
void* arena_alloc(Arena* arena, size_t capacity);

// zero-initialized
void* arena_realloc(Arena* arena, void* old_buf, size_t old_capacity, size_t new_capacity);

// reset, but do not free, allocated area
void arena_reset(Arena* arena);

void arena_destroy(Arena* arena);

size_t arena_get_total_capacity(const Arena* arena);

#endif // ARENA_H
