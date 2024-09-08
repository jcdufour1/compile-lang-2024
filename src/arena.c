#include "arena.h"
#include "util.h"
#include "assert.h"
#include <stddef.h>

#define ARENA_DEFAULT_CAPACITY (1 << 20) // 1 MB initial
#define ARENA_FREE_NODE_BLOCK_CAPACITY 16

static void* safe_realloc(void* old_ptr, size_t old_capacity, size_t new_count_items, size_t size_each_item) {
    size_t new_capacity = new_count_items*size_each_item;
    void* new_ptr = realloc(old_ptr, new_capacity);
    if (!new_ptr) {
        msg(LOG_FETAL, "", 0, "realloc failed\n");
        exit(1);
    }
    memset((char*)new_ptr + old_capacity, 0,  new_capacity - old_capacity);
    return new_ptr;
}

// buffer is zero initialized
static void* safe_malloc(size_t capacity) {
    void* new_ptr = malloc(capacity);
    if (!new_ptr) {
        msg(LOG_FETAL, "", 0, "malloc failed\n");
        exit(1);
    }
    memset(new_ptr, 0, capacity);
    return new_ptr;
}

#define safe_free(ptr) \
    do { \
        free(ptr); \
        (ptr) = NULL; \
    } while (0);

#ifdef NDEBUG
#define push_alloc(buf, capacity_needed)
#define assert_node_alloced(buf, capacity_needed)
#define remove_alloc(buf)
#else
static bool buf_is_in_alloc(const void* buf) {
    Arena_alloc_node* curr_node = arena.alloc_node;
    while (curr_node) {
        assert(curr_node->buf);
        if ((size_t)curr_node->buf == (size_t)buf) {
            return true;
        }

        curr_node = curr_node->next;
    }

    return false;
}
static bool node_is_alloced(const void* buf, size_t old_capacity) {
    Arena_alloc_node* curr_node = arena.alloc_node;
    while (curr_node) {
        assert(curr_node->buf);
        if ((size_t)curr_node->buf == (size_t)buf) {
            if (curr_node->capacity != old_capacity) {
                return false;
            }
            return true;
        }

        curr_node = curr_node->next;
    }

    return false;
}

static void push_alloc(void* buf, size_t capacity_needed) {
    size_t init_count_nodes = arena_count_allocated_nodes();
    assert(!node_is_alloced(buf, capacity_needed) && "buf already alloced");
    assert(buf);
    Arena_alloc_node* curr_node = arena.alloc_node;
    if (!curr_node) {
        arena.alloc_node = safe_malloc(sizeof(*arena.alloc_node));
        arena.alloc_node->buf = buf;
        arena.alloc_node->capacity = capacity_needed;
        assert(node_is_alloced(buf, capacity_needed));
        assert(init_count_nodes + 1 == arena_count_allocated_nodes());
        return;
    }

    Arena_alloc_node* prev_node = NULL;
    while (curr_node) {
        prev_node = curr_node;
        curr_node = curr_node->next;
    }
    assert(!buf_is_in_alloc(buf));
    prev_node->next = safe_malloc(sizeof(*arena.alloc_node));
    prev_node->next->buf = buf;
    prev_node->next->capacity = capacity_needed;
    assert(node_is_alloced(buf, capacity_needed));
    assert(init_count_nodes + 1 == arena_count_allocated_nodes());
}

static void remove_alloc(void* buf) {
    size_t init_count_nodes = arena_count_allocated_nodes();
    Arena_alloc_node* curr_node = arena.alloc_node;
    Arena_alloc_node* prev_node = NULL;
    while (curr_node) {
        if (curr_node->buf == buf) {
            if (prev_node) {
                prev_node->next = curr_node->next;
            } else {
                arena.alloc_node = curr_node->next;
            }
            safe_free(curr_node);
            assert(init_count_nodes == arena_count_allocated_nodes() + 1);
            return;
        }
        prev_node = curr_node;
        curr_node = curr_node->next;
    }
    unreachable("");
}
#endif // NDEBUG

static size_t space_remaining(const Arena_buf arena_buf) {
    return arena_buf.capacity - arena_buf.in_use;
}

// TODO: combine contiguous free nodes
static bool use_free_node(void** buf, size_t capacity_needed) {
    Arena_free_node* prev_node = NULL;
    Arena_free_node* curr_node = arena.free_node;
    while (curr_node) {
        if (curr_node->capacity >= capacity_needed) {
            //arena_log_alloced_nodes();
            //arena_log_free_nodes();
            assert(!buf_is_in_alloc(curr_node->buf));
            assert(!node_is_alloced(curr_node->buf, curr_node->capacity));
            *buf = curr_node->buf;
            memset(*buf, 0, capacity_needed);
            // create smaller node
            curr_node->buf = (char*)curr_node->buf + capacity_needed;
            curr_node->capacity -= capacity_needed;
            return true;
        }

        prev_node = curr_node;
        curr_node = curr_node->next;
        assert(prev_node != curr_node);
    }

    return false;
}

static size_t get_total_alloced(void) {
    Arena_buf* curr_buf = arena.buf;
    size_t total_alloced = 0;
    while (curr_buf) {
        total_alloced += curr_buf->capacity;
        curr_buf = curr_buf->next;
    }
    return total_alloced;
}

void* arena_alloc(size_t capacity_needed) {
    {
        void* buf;
        if (use_free_node(&buf, capacity_needed)) {
            assert(!buf_is_in_alloc(buf));
            assert(!node_is_alloced(buf, capacity_needed));
            // a free node can hold this item
            assert(0 == memcmp(buf, zero_block, capacity_needed));
            push_alloc(buf, capacity_needed);
            assert(node_is_alloced(buf, capacity_needed));
            return buf;
        }
    }

    // no free node could hold this item
    Arena_buf* arena_buf_new_region = arena.buf;
    Arena_buf* arena_buf_last = NULL;
    while (arena_buf_new_region) {
        arena_buf_last = arena_buf_new_region;
        if (space_remaining(*arena_buf_new_region) >= capacity_needed) {
            break;
        }
        arena_buf_new_region = arena_buf_new_region->next;
    }

    if (!arena_buf_new_region) {
        // could not find space; must allocate new Arena_buf
        size_t arena_buf_capacity = MAX(get_total_alloced(), ARENA_DEFAULT_CAPACITY);
        arena_buf_capacity = MAX(arena_buf_capacity, capacity_needed + sizeof(*arena.buf));
        arena_buf_new_region = safe_malloc(arena_buf_capacity);
        arena_buf_new_region->buf_after_taken = arena_buf_new_region + 1;
        arena_buf_new_region->in_use = sizeof(*arena.buf);
        arena_buf_new_region->capacity = arena_buf_capacity;
        if (arena_buf_last) {
            arena_buf_last->next = arena_buf_new_region;
        } else {
            arena.buf = arena_buf_new_region;
        }
    }

    assert(arena_buf_new_region);
    void* new_alloc_buf = arena_buf_new_region->buf_after_taken;
    memset(new_alloc_buf, 0, capacity_needed);
    arena_buf_new_region->in_use += capacity_needed;
    arena_buf_new_region->buf_after_taken = (char*)arena_buf_new_region->buf_after_taken + capacity_needed;
    assert(0 == memcmp(new_alloc_buf, zero_block, capacity_needed));
    push_alloc(new_alloc_buf, capacity_needed);
    assert(node_is_alloced(new_alloc_buf, capacity_needed));
    return new_alloc_buf;
}

static bool find_empty_free_node(Arena_free_node** result) {
    Arena_free_node* curr_free_node = arena.free_node;
    while (curr_free_node) {
        if (curr_free_node->capacity < 1) {
            *result = curr_free_node;
            return true;
        }

        curr_free_node = curr_free_node->next;
    }

    return false;
}

static void join_two_free_nodes_if_possible(Arena_free_node* node_to_join) {
    Arena_free_node* curr_node = arena.free_node;
    while (curr_node) {
        if ((char*)node_to_join->buf + node_to_join->capacity == curr_node->buf) {
            node_to_join->capacity += curr_node->capacity;
            curr_node->capacity = 0;
        }

        curr_node = curr_node->next;
    }
}

static bool expand_neighbor_free_node(void* buf, size_t capacity) {
    Arena_free_node* curr_node = arena.free_node;
    while (curr_node) {
        if ((char*)curr_node->buf + curr_node->capacity == (char*)buf) {
            curr_node->capacity += capacity;
            join_two_free_nodes_if_possible(curr_node);
            return true;
        }

        if (curr_node && (char*)curr_node->buf == (char*)buf + capacity) {
            curr_node->buf = buf;
            curr_node->capacity += capacity;
            return true;
        }

        curr_node = curr_node->next;
    }

    return false;
}

void arena_free(void* buf, size_t old_capacity) {
    assert(buf && "null freed");
    assert(buf_is_in_alloc(buf));
    assert(node_is_alloced(buf, old_capacity));
    remove_alloc(buf);
    assert(!buf_is_in_alloc(buf));
    assert(!node_is_alloced(buf, old_capacity));

    if (expand_neighbor_free_node(buf, old_capacity)) {
        return;
    }

    Arena_free_node* free_node;
    if (find_empty_free_node(&free_node)) {
        free_node->buf = buf;
        free_node->capacity = old_capacity;
        return;
    }

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
    assert(!node_is_alloced(buf, old_capacity));
}

void* arena_realloc(void* old_buf, size_t old_capacity, size_t new_capacity) {
    void* new_buf = arena_alloc(new_capacity);
    memcpy(new_buf, old_buf, old_capacity);
    arena_free(old_buf, old_capacity);
    return new_buf;
}

void arena_destroy(void) {
    todo();
#ifndef NDEBUG
    Arena_alloc_node* curr_node = arena.alloc_node;
    Arena_alloc_node* prev_node = NULL;
    while (curr_node) {
        prev_node = curr_node;
        curr_node = curr_node->next;
        safe_free(prev_node);
    }
#endif // NDEBUG
}

size_t arena_get_total_capacity(void) {
    size_t total = 0;
    Arena_buf* curr_buf = arena.buf;
    while (curr_buf) {
        total += curr_buf->capacity;
        curr_buf = curr_buf->next;
    }
    return total;
}

