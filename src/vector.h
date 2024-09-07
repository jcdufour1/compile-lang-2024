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

static inline void vector_reserve(void* typed_vector, size_t size_each_item, size_t minimum_count_empty_slots, size_t init_capacity) {
    Vector* vector = (Vector*)typed_vector;

    while (vector->info.count + minimum_count_empty_slots + 1 > vector->info.capacity) {
        if (vector->info.capacity < 1) {
            vector->info.capacity = init_capacity;
            log(LOG_NOTE, "%zu\n", vector->info.capacity*size_each_item);
            vector->buf = arena_alloc(vector->info.capacity*size_each_item);
        } else {
            size_t old_capacity_count_elements = vector->info.capacity;
            log(LOG_NOTE, "%zu\n", vector->info.capacity*size_each_item);
            vector->info.capacity *= 2;
            vector->buf = arena_realloc(vector->buf, old_capacity_count_elements*size_each_item, vector->info.capacity*size_each_item);
        }
    }
}

static inline void vector_append(void* typed_vector, size_t size_each_item, const void* item_to_append, size_t init_capacity) {
    Vector* vector = (Vector*)typed_vector;

    vector_reserve(typed_vector, size_each_item, 2, init_capacity);
    memmove((char*)vector->buf + size_each_item*vector->info.count, item_to_append, size_each_item);
    vector->info.count++;
}

static inline void vector_set_to_zero_len(void* typed_vector, size_t size_each_item) {
    Vector* vector = (Vector*)typed_vector;

    memset(vector->buf, 0, size_each_item*vector->info.count);
    vector->info.count = 0;
}

static inline bool vector_in(void* typed_vector, size_t size_each_item, const void* item_to_find) {
    Vector* vector = (Vector*)typed_vector;

    for (size_t idx = 0; idx < vector->info.count; idx++) {
        if (0 == memcmp((char*)vector->buf + size_each_item*idx, item_to_find, size_each_item)) {
            return true;
        }
    }
    return false;
}

#endif // VECTOR_H
