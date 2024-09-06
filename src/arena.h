#ifndef ARENA_H
#define ARENA_H

#include <stddef.h>

typedef struct Arena_free_node_ {
    void* buf;
    size_t capacity;
    struct Arena_free_node_* next;
} Arena_free_node;

typedef struct {
    void* buf_after_taken;
    size_t in_use;
    size_t capacity; // size in bytes of space malloced

    Arena_free_node* free_node;
} Arena;

extern Arena arena;

// allocate a zero-initialized memory region in arena, return pointer (similer to malloc, but never return null)
void* arena_alloc(size_t capacity);

// zero-initialized
void* arena_realloc(void* old_buf, size_t old_capacity, size_t new_capacity);

void arena_free(void* buf, size_t old_capacity);

#endif // ARENA_H
