#ifndef ARENA_H
#define ARENA_H

#include <stddef.h>

typedef struct {
    void* buf;
    size_t capacity;
} Arena_free_node;

typedef struct {
    void* buf;
    size_t in_use; // size in bytes of space taken
    size_t capacity; // size in bytes of space malloced

    Arena_free_node* free_nodes;
    size_t free_nodes_count;
    size_t free_nodes_capacity;
} Arena;

extern Arena arena;

// allocate a memory region in arena, return pointer (similer to malloc, but never return null)
void* arena_alloc(size_t capacity);

void* arena_realloc(void* old_buf, size_t old_capacity, size_t new_capacity);

void arena_free(void* buf, size_t old_capacity);

#endif // ARENA_H
