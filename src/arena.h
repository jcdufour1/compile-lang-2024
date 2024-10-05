#ifndef ARENA_H
#define ARENA_H

#include <stddef.h>
#include <stdbool.h>
#include "util.h"

typedef struct Arena_buf_ {
    void* buf_after_taken; // this pointer is incremented as space is used up
    size_t in_use; // buf_after_taken - in_use will yield the start of the buffer (including Arena_buf_ itself)
    size_t capacity; // size in bytes of space malloced
    struct Arena_buf_* next;
} Arena_buf;

typedef struct {
    Arena_buf* buf;
} Arena;

extern Arena a_main;
extern Arena print_arena;

extern void* arena_buffers[100000];
extern size_t arena_buffers_count;

// allocate a zero-initialized memory region in arena, return pointer (similer to malloc, but never return null)
void* arena_alloc(Arena* arena, size_t capacity);

// zero-initialized
void* arena_realloc(Arena* arena, void* old_buf, size_t old_capacity, size_t new_capacity);

// reset, but do not free, allocated area
void arena_reset(Arena* arena);

void arena_destroy(Arena* arena);

size_t arena_get_total_capacity(const Arena* arena);

#endif // ARENA_H
