#ifndef ARENA_H
#define ARENA_H

#include <stddef.h>

typedef struct Arena_buf_ {
    void* buf_after_taken;
    size_t in_use;
    size_t capacity; // size in bytes of space malloced
    struct Arena_buf_* next;
} Arena_buf;

typedef struct Arena_free_node_ {
    void* buf;
    size_t capacity;
    struct Arena_free_node_* next;
} Arena_free_node;

typedef struct {
    Arena_buf* buf;
    Arena_free_node* free_node;
} Arena;

extern Arena arena;

extern void* arena_buffers[100000];
extern size_t arena_buffers_count;

// allocate a zero-initialized memory region in arena, return pointer (similer to malloc, but never return null)
void* arena_alloc(size_t capacity);

// zero-initialized
void* arena_realloc(void* old_buf, size_t old_capacity, size_t new_capacity);

void arena_free(void* buf, size_t old_capacity);

#define arena_log_free_nodes() \
    do { \
        log(LOG_DEBUG, "thing 489\n"); \
        Arena_free_node* curr_node = arena.free_node; \
        while (curr_node) { \
            log(LOG_DEBUG, "%p %zu\n", curr_node->buf, curr_node->capacity); \
            curr_node = curr_node->next; \
        } \
    } \
    while (0)

#endif // ARENA_H
