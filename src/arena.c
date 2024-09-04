#include "arena.h"
#include "util.h"
#include <stddef.h>

#define ARENA_DEFAULT_CAPACITY (1 << 20) // 1 MB initial

static size_t space_remaining() {
    return arena.capacity - arena.in_use;
}

void* arena_alloc(size_t count_items, size_t size_each_item) {
    size_t capacity_needed = count_items*size_each_item;

    if (arena.capacity < 1) {
        arena.capacity = ARENA_DEFAULT_CAPACITY;
        while (space_remaining() < capacity_needed) {
            arena.capacity *= 2;
        }
        arena.buf = safe_malloc(count_items, size_each_item);
        return arena.buf;
    }

    if (space_remaining() < capacity_needed) {
        todo();
    }

    todo();
}
