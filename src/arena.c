#include "arena.h"
#include "util.h"
#include "assert.h"
#include <stddef.h>

#define ARENA_DEFAULT_CAPACITY (1 << 20) // 1 MB initial

static void* safe_realloc(void* old_ptr, size_t old_capacity, size_t new_count_items, size_t size_each_item) {
    size_t new_capacity = new_count_items*size_each_item;
    void* new_ptr = realloc(old_ptr, new_capacity);
    if (!new_ptr) {
        msg(LOG_FETAL, dummy_pos, "realloc failed\n");
        exit(1);
    }
    memset((char*)new_ptr + old_capacity, 0, new_capacity - old_capacity);
    return new_ptr;
}

// buffer is zero initialized
static void* safe_malloc(size_t capacity) {
    void* new_ptr = malloc(capacity);
    if (!new_ptr) {
        msg(LOG_FETAL, dummy_pos, "malloc failed\n");
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

#if 1
#define push_alloc(arena, buf, capacity_needed)
#define assert_node_alloced(buf, capacity_needed)
#define remove_alloc(buf)
#define buf_is_in_alloc(buf) true
#define node_is_alloced(buf, capacity) true
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

static size_t get_total_alloced(void) {
    Arena_buf* curr_buf = arena.buf;
    size_t total_alloced = 0;
    while (curr_buf) {
        total_alloced += curr_buf->capacity;
        curr_buf = curr_buf->next;
    }
    return total_alloced;
}

void* arena_alloc(Arena* arena, size_t capacity_needed) {
    Arena_buf* arena_buf_new_region = arena->buf;
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
        arena_buf_capacity = MAX(arena_buf_capacity, capacity_needed + sizeof(*arena->buf));
        arena_buf_new_region = safe_malloc(arena_buf_capacity);
        arena_buf_new_region->buf_after_taken = arena_buf_new_region + 1;
        arena_buf_new_region->in_use = sizeof(*arena->buf);
        arena_buf_new_region->capacity = arena_buf_capacity;
        if (arena_buf_last) {
            arena_buf_last->next = arena_buf_new_region;
        } else {
            arena->buf = arena_buf_new_region;
        }
    }

    assert(arena_buf_new_region);
    void* new_alloc_buf = arena_buf_new_region->buf_after_taken;
    memset(new_alloc_buf, 0, capacity_needed);
    arena_buf_new_region->in_use += capacity_needed;
    arena_buf_new_region->buf_after_taken = (char*)arena_buf_new_region->buf_after_taken + capacity_needed;
    assert(0 == memcmp(new_alloc_buf, zero_block, capacity_needed));
    push_alloc(arena, new_alloc_buf, capacity_needed);
    assert(node_is_alloced(new_alloc_buf, capacity_needed));
    return new_alloc_buf;
}

void* arena_realloc(Arena* arena, void* old_buf, size_t old_capacity, size_t new_capacity) {
    void* new_buf = arena_alloc(arena, new_capacity);
    memcpy(new_buf, old_buf, old_capacity);
    return new_buf;
}

void arena_destroy(Arena* arena) {
    (void) arena;
    todo();
#if 0
    Arena_alloc_node* curr_node = arena.alloc_node;
    Arena_alloc_node* prev_node = NULL;
    while (curr_node) {
        prev_node = curr_node;
        curr_node = curr_node->next;
        safe_free(prev_node);
    }
#endif
}

// reset, but do not free, allocated area
void arena_reset(Arena* arena) {
    Arena_buf* curr_buf = arena->buf;
    while (curr_buf) {
        assert(curr_buf->in_use >= sizeof(*arena->buf));
        curr_buf->buf_after_taken = (char*)curr_buf->buf_after_taken - (curr_buf->in_use - sizeof(*arena->buf));
        memset(
            (char*)curr_buf->buf_after_taken + sizeof(*arena->buf),
            0,
            curr_buf->in_use - sizeof(*arena->buf)
        );
        curr_buf->in_use = sizeof(*arena->buf);
        curr_buf = curr_buf->next;
    }
}

size_t arena_get_total_capacity(Arena* arena) {
    size_t total = 0;
    Arena_buf* curr_buf = arena->buf;
    while (curr_buf) {
        total += curr_buf->capacity;
        curr_buf = curr_buf->next;
    }
    return total;
}

