#ifndef VECTOR_H
#define VECTOR_H

#include <stddef.h>
#include "util.h"
#include "arena.h"

typedef struct {
    size_t count;
    size_t capacity;
} Vec_base;

typedef struct {
    Vec_base info;
    void* buf;
} Vector;

static inline void vector_reserve(
    Arena* arena,
    void* typed_vector,
    size_t size_each_item,
    size_t minimum_count_empty_slots,
    size_t init_capacity
) {
    Vector* vector = typed_vector;

    while (vector->info.count + minimum_count_empty_slots + 1 > vector->info.capacity) {
        if (vector->info.capacity < 1) {
            vector->info.capacity = init_capacity;
            vector->buf = arena_alloc(arena, vector->info.capacity*size_each_item);
        } else {
            size_t old_capacity_count_elements = vector->info.capacity;
            vector->info.capacity *= 2;
            vector->buf = arena_realloc(arena, vector->buf, old_capacity_count_elements*size_each_item, vector->info.capacity*size_each_item);
        }
    }
}

static inline void vector_append(Arena* arena,
    void* typed_vector,
    size_t size_each_item,
    const void* item_to_append,
    size_t init_capacity
) {
    Vector* vector = typed_vector;

    vector_reserve(arena, typed_vector, size_each_item, 2, init_capacity);
    memmove((char*)vector->buf + size_each_item*vector->info.count, item_to_append, size_each_item);
    vector->info.count++;
}

static inline void vector_extend(
    Arena* arena,
    void* typed_dest,
    const void* typed_src,
    size_t size_each_item,
    size_t init_capacity
) {
    Vector* dest = typed_dest;
    const Vector* src = typed_src;

    for (size_t idx = 0; idx < src->info.count; idx++) {
        const void* item_to_append = (char*)src->buf + size_each_item*idx;
        vector_append(arena, dest, size_each_item, item_to_append, init_capacity);
    }
}

static inline void vector_insert(
    Arena* arena,
    void* typed_vector,
    size_t index,
    size_t size_each_item,
    const void* item_to_append,
    size_t init_capacity
) {
    Vector* vector = typed_vector;

    // make room for item to be inserted
    vector_reserve(arena, typed_vector, size_each_item, 2, init_capacity);
    size_t count_to_move = vector->info.count - index;
    memmove((char*)vector->buf + size_each_item*(index + 1), (char*)vector->buf + size_each_item*index, size_each_item*count_to_move);

    // actually insert item
    memmove((char*)vector->buf + size_each_item*index, item_to_append, size_each_item);

    vector->info.count++;
}

static inline void vector_set_to_zero_len(void* typed_vector, size_t size_each_item) {
    Vector* vector = typed_vector;

    memset(vector->buf, 0, size_each_item*vector->info.count);
    vector->info.count = 0;
}

static inline bool vector_in(void* typed_vector, size_t size_each_item, const void* item_to_find) {
    Vector* vector = typed_vector;

    for (size_t idx = 0; idx < vector->info.count; idx++) {
        if (0 == memcmp((char*)vector->buf + size_each_item*idx, item_to_find, size_each_item)) {
            return true;
        }
    }
    return false;
}

#endif // VECTOR_H
