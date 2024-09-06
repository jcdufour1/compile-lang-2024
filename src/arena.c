#include "arena.h"
#include "util.h"
#include "assert.h"
#include <stddef.h>

#define ARENA_DEFAULT_CAPACITY (1 << 20) // 1 MB initial
#define ARENA_FREE_NODE_BLOCK_CAPACITY 16

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

static size_t space_remaining(void) {
    return arena.capacity - arena.in_use;
}

static void remove_free_node(Arena_free_node* node_to_remove) {
    assert(node_to_remove && "node_to_remove should not be null");

    if (node_to_remove == (Arena_free_node*)node_to_remove->buf) {
        todo();
    }

    Arena_free_node new_node = {.buf = node_to_remove, .capacity = sizeof(*node_to_remove), .next = node_to_remove->next};
    *node_to_remove = new_node;
    assert(node_to_remove != node_to_remove->next);
}

// TODO: combine contiguous free nodes
static bool use_free_node(void** buf, size_t capacity_needed) {
    Arena_free_node* prev_node = NULL;
    Arena_free_node* curr_node = arena.free_node;
    while (curr_node) {
        if (curr_node->capacity >= capacity_needed) {
            *buf = curr_node->buf;
            memset(*buf, 0, capacity_needed);
            if (curr_node->capacity > capacity_needed) {
                // create smaller node
                curr_node->buf = (char*)curr_node->buf + capacity_needed;
                curr_node->capacity -= capacity_needed;
            } else {
                // remove free node
                remove_free_node(curr_node);
            }
            return true;
        }

        prev_node = curr_node;
        curr_node = curr_node->next;
        assert(prev_node != curr_node);
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
        arena.buf_after_taken = safe_malloc(arena.capacity);
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
    void* new_alloc_buf = arena.buf_after_taken;
    memset(new_alloc_buf, 0, capacity_needed);
    log(LOG_DEBUG, "thing 89: %p %zu\n", new_alloc_buf, capacity_needed);
    arena.in_use += capacity_needed;
    arena.buf_after_taken = arena.buf_after_taken + capacity_needed;
    return new_alloc_buf;
}

void arena_free(void* buf, size_t old_capacity) {
    assert(buf && "null freed");

    Arena_free_node* new_next = arena.free_node ? (arena.free_node->next) : NULL;
    Arena_free_node* new_node = arena_alloc(sizeof(*arena.free_node));

    if (new_node == arena.free_node) {
        new_next = arena.free_node->next;
    } else {
        new_next = arena.free_node;
    }
    Arena_free_node new_node_struct = {.buf = buf, .capacity = old_capacity, .next = new_next};
    *new_node = new_node_struct;
    arena.free_node = new_node;
    assert(arena.free_node != arena.free_node->next);
}

void* arena_realloc(void* old_buf, size_t old_capacity, size_t new_capacity) {
    void* new_buf = arena_alloc(new_capacity);
    memcpy(new_buf, old_buf, old_capacity);
    arena_free(old_buf, old_capacity);
    return new_buf;
}
