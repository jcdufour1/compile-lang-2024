#include "arena.h"
#include "util.h"
#include "assert.h"
#include <stddef.h>

#define ARENA_DEFAULT_CAPACITY (1 << 20) // 1 MB initial
#define ARENA_FREE_NODES_DEFAULT_CAPACITY 16 // 16 free nodes initial

static size_t space_remaining(void) {
    return arena.capacity - arena.in_use;
}

static void* safe_realloc(void* old_ptr, size_t old_capacity, size_t new_count_items, size_t size_each_item) {
    size_t new_capacity = new_count_items*size_each_item;
    void* new_ptr = realloc(old_ptr, new_capacity);
    memset((char*)new_ptr + old_capacity, 0,  new_capacity - old_capacity);
    if (!new_ptr) {
        todo();
    }
    return new_ptr;
}

// buffer is zero initialized
static void* safe_malloc(size_t capacity) {
    void* new_ptr = malloc(capacity);
    if (!new_ptr) {
        todo();
    }
    memset(new_ptr, 0, capacity);
    return new_ptr;
}

#define safe_free(ptr) \
    do { \
        free(ptr); \
        (ptr) = NULL; \
    } while (0);

static void remove_free_node(Arena_free_node* node_ptr) {
    size_t count_nodes_to_move = (arena.free_nodes + arena.free_nodes_count) - (node_ptr + 1);
    memmove(node_ptr, node_ptr + 1, count_nodes_to_move);
    arena.free_nodes_count--;
}

static bool use_free_node(void** buf, size_t capacity_needed) {
    for (size_t idx = 0; idx < arena.free_nodes_count; idx++) {
        if (arena.free_nodes[idx].capacity >= capacity_needed) {
            if (arena.free_nodes[idx].capacity > capacity_needed) {
                *buf = arena.free_nodes[idx].buf;
                memset(*buf, 0, capacity_needed);
                // create smaller node
                arena.free_nodes[idx].buf += capacity_needed;
                arena.free_nodes[idx].capacity -= capacity_needed;
                return true;
            }

            *buf = arena.free_nodes[idx].buf;
            memset(*buf, 0, capacity_needed);
            remove_free_node(&arena.free_nodes[idx]);
            return true;
        }
    }

    return false;
}

void* arena_alloc(size_t capacity_needed) {
    // TODO: get region from free list if possible

    if (arena.capacity < 1) {
        arena.capacity = ARENA_DEFAULT_CAPACITY;
        while (space_remaining() < capacity_needed) {
            arena.capacity *= 2;
        }
        arena.buf = safe_malloc(arena.capacity);
        log(LOG_DEBUG, "thing 88: %p %zu\n", arena.buf, arena.capacity);
    }

    if (space_remaining() < capacity_needed) {
        // need to allocate new region
        todo();
    }

    void* buf;
    if (use_free_node(&buf, capacity_needed)) {
        // a free node can hold this item
        return buf;
    }

    // no free node could hold this item
    void* new_alloc_buf = (char*)arena.buf + arena.in_use;
    memset(new_alloc_buf, 0, capacity_needed);
    log(LOG_DEBUG, "thing 89: %p %zu\n", new_alloc_buf, capacity_needed);
    arena.in_use += capacity_needed;
    return new_alloc_buf;
}

void arena_free(void* buf, size_t old_capacity) {
    assert(buf && "null freed");

    if (arena.free_nodes_capacity < 1) {
        arena.free_nodes_capacity = ARENA_FREE_NODES_DEFAULT_CAPACITY;
        arena.free_nodes = arena_alloc(arena.free_nodes_capacity*sizeof(arena.free_nodes[0]));
        log(LOG_DEBUG, "thing 98: %p\n", (void*)arena.free_nodes);
    }

    log(LOG_DEBUG, "thing 100: %p\n", (void*)buf);
    if (arena.free_nodes_capacity <= arena.free_nodes_count) {
        // need to expand free_nodes
        todo();
    }

    Arena_free_node new_node = {.buf = buf, .capacity = old_capacity};
    arena.free_nodes[arena.free_nodes_count] = new_node;
    arena.free_nodes_count++;
}

void* arena_realloc(void* old_buf, size_t old_capacity, size_t new_capacity) {
    void* new_buf = arena_alloc(new_capacity);
    memcpy(new_buf, old_buf, old_capacity);
    arena_free(old_buf, old_capacity);
    return new_buf;
}
