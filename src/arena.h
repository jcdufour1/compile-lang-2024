#ifndef ARENA_H
#define ARENA_H

#include <stddef.h>

typedef struct {
    void* buf;
    size_t in_use; // size in bytes of space taken
    size_t capacity; // size in bytes of space malloced
} Arena;

extern Arena arena;

// allocate a memory region in arena, return pointer (similer to malloc, but never return null)
void* arena_alloc(size_t count, size_t size_each_item);

#endif // ARENA_H
