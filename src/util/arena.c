#include <arena.h>
#include <util.h>
#include <assert.h>
#include <stddef.h>

// TODO: use mmap instead of malloc, etc.

typedef union {
    size_t a;
    uint64_t b;
    void* c;
    long double d;
} Align_thing;

#define ALIGN_SIZE (sizeof(Align_thing))

#ifdef NDEBUG
#define ARENA_DEFAULT_CAPACITY (ALIGN_SIZE*((1 << 20)/ALIGN_SIZE)) // 1 MB initial
#else
#define ARENA_DEFAULT_CAPACITY ALIGN_SIZE // this is to catch bugs with the arena, strv functions, etc.
#endif // NDEBUG

/*
static void* safe_realloc(void* old_ptr, size_t old_capacity, size_t new_count_items, size_t size_each_item) {
    size_t new_capacity = new_count_items*size_each_item;
    void* new_ptr = realloc(old_ptr, new_capacity);
    if (!new_ptr) {
        fprintf(stderr, "realloc failed\n");
        exit(EXIT_CODE_FAIL);
    }
    memset((char*)new_ptr + old_capacity, 0, new_capacity - old_capacity);
    return new_ptr;
}
*/

// buffer is zero initialized
static void* safe_malloc(size_t capacity) {
    void* new_ptr = malloc(capacity);
    if (!new_ptr) {
        fprintf(stderr, "malloc failed\n");
        exit(EXIT_CODE_FAIL);
    }
    memset(new_ptr, 0, capacity);
    assert((uint64_t)new_ptr%ALIGN_SIZE == 0 && "new_ptr is not aligned");
    return new_ptr;
}

#define safe_free(ptr) \
    do { \
        free(ptr); \
        (ptr) = NULL; \
    } while (0);

static size_t get_total_alloced(Arena* arena) {
    Arena_buf* curr_buf = arena->next;
    size_t total_alloced = 0;
    while (curr_buf) {
        total_alloced += curr_buf->capacity;
        curr_buf = curr_buf->next;
    }
    return total_alloced;
}

void* arena_alloc(Arena* arena, size_t capacity_needed) {
    capacity_needed = get_next_multiple(capacity_needed, ALIGN_SIZE);
    size_t arena_buf_size = get_next_multiple(sizeof(Arena_buf), ALIGN_SIZE);

    Arena_buf** curr_buf = &arena->next;
    while (1) {
        if (!(*curr_buf)) {
            size_t cap_new_buf = MAX(get_total_alloced(arena), ARENA_DEFAULT_CAPACITY);
            cap_new_buf = get_next_multiple(MAX(cap_new_buf, capacity_needed + arena_buf_size), ALIGN_SIZE);
            *curr_buf = safe_malloc(cap_new_buf);
            (*curr_buf)->capacity = cap_new_buf;
            (*curr_buf)->count = arena_buf_size;
        }
        size_t rem_capacity = (*curr_buf)->capacity - (*curr_buf)->count;
        if (rem_capacity >= capacity_needed) {
            break;
        }

        curr_buf = &(*curr_buf)->next;
    }

    void* buf_to_return = (char*)(*curr_buf) + (*curr_buf)->count;
    (*curr_buf)->count += capacity_needed;
    assert((uint64_t)buf_to_return%ALIGN_SIZE == 0 && "buf_to_return is not actually aligned");
    return buf_to_return;
}

void* arena_realloc(Arena* arena, void* old_buf, size_t old_capacity, size_t new_capacity) {
    void* new_buf = arena_alloc(arena, new_capacity);
    memcpy(new_buf, old_buf, old_capacity);
    return new_buf;
}

INLINE void arena_free_buf(Arena_buf* buf) {
    Arena_buf* curr = buf;
    while (curr) {
        Arena_buf* next = curr->next;
        safe_free(curr);
        curr = next;
    }
}

void arena_free_internal(Arena* arena) {
    if (!arena) {
        return;
    }
    arena_free_buf(arena->next);
}

// reset, but do not free, allocated area
void arena_reset(Arena* arena) {
    Arena_buf* curr_buf = arena->next;
    while (curr_buf) {
        // memset nessessary so that reused buffers will be zero initialized
        memset(curr_buf + 1, 0, curr_buf->count - sizeof(*curr_buf));
        curr_buf->count = get_next_multiple(sizeof(*curr_buf), ALIGN_SIZE);

        curr_buf = curr_buf->next;
    }
}

size_t arena_get_total_capacity(const Arena* arena) {
    size_t total = 0;
    Arena_buf* curr_buf = arena->next;
    while (curr_buf) {
        total += curr_buf->capacity;
        curr_buf = curr_buf->next;
    }
    return total;
}

size_t arena_get_total_usage(const Arena* arena) {
    size_t total = 0;
    Arena_buf* curr_buf = arena->next;
    while (curr_buf) {
        total += curr_buf->count - sizeof(*curr_buf);
        curr_buf = curr_buf->next;
    }
    return total;
}

