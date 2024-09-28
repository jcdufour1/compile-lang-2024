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

typedef struct Arena_free_node_ {
    void* buf;
    size_t capacity;
} Arena_free_node;

#ifndef NDEBUG
typedef Arena_free_node Arena_alloc_node;
#endif // NDEBUG

typedef struct {
    Arena_buf* buf;
    Arena_free_node* free_node;
#if 0
    Arena_alloc_node* alloc_node;
#endif
} Arena;

extern Arena arena;
extern Arena print_arena;

extern void* arena_buffers[100000];
extern size_t arena_buffers_count;

// allocate a zero-initialized memory region in arena, return pointer (similer to malloc, but never return null)
void* arena_alloc(Arena* arena, size_t capacity);

// zero-initialized
void* arena_realloc(Arena* arena, void* old_buf, size_t old_capacity, size_t new_capacity);

void arena_destroy(Arena* arena);

#if 0
static inline size_t arena_count_allocated_nodes(void) {
    size_t count_nodes = 0;
    Arena_alloc_node* curr_node = arena.alloc_node;
    while (curr_node) {
        count_nodes++;
        curr_node = curr_node->next;
    }
    return count_nodes;
}

#define arena_log_alloced_nodes() \
    do { \
        size_t count_nodes = 0; \
        Arena_alloc_node* curr_node = arena.alloc_node; \
        while (curr_node) { \
            count_nodes++; \
            log(LOG_DEBUG, "%p %zu\n", curr_node->buf, curr_node->capacity); \
            curr_node = curr_node->next; \
        } \
        log(LOG_DEBUG, "total count alloc nodes printed: %zu\n", count_nodes); \
    } \
    while (0)

#endif

// reset, but do not free, allocated area
void arena_reset(Arena* arena);

size_t arena_get_total_capacity(Arena* arena);

#endif // ARENA_H
