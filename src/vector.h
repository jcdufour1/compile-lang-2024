#ifndef VECTOR_H
#define VECTOR_H

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include "util.h"
#include "arena.h"

typedef struct {
    size_t count;
    size_t capacity;
} Vec_base;

#define vec_reserve(arena, vector, min_count_empty_slots) \
    do { \
        size_t original_capacity = (vector)->info.capacity; \
        size_t needs_resizing = false; \
        while ((vector)->info.capacity < (vector)->info.count + (min_count_empty_slots)) { \
            if ((vector)->info.capacity < 1) { \
                (vector)->info.capacity = 2; \
            } else { \
                needs_resizing = true; \
                (vector)->info.capacity *= 2; \
            } \
        } \
        if (original_capacity == 0) { \
            (vector)->buf = arena_alloc(arena, sizeof((vector)->buf[0])*(vector)->info.capacity); \
        } else if (needs_resizing) { \
            (vector)->buf = arena_realloc(arena, (vector)->buf, sizeof((vector)->buf[0])*original_capacity, sizeof((vector)->buf[0])*(vector)->info.capacity); \
        } \
    } while(0)

#define vec_append(arena, vector, item_to_append) \
    do { \
        (void) (arena); \
        vec_reserve(arena, vector, 1); \
        (vector)->buf[(vector)->info.count++] = (item_to_append); \
    } while(0)

#define vec_reset(vector) \
    do { \
        (vector)->info.count = 0; \
    } while(0)

#define vec_at(vector, index) \
    ((vector)->buf[(index)])

#define vec_at_ref(vector, index) \
    (&(vector)->buf[(index)])

#define vec_insert(arena, vector, index, item_to_insert) \
    do { \
        vec_reserve(arena, vector, 1); \
        memmove((vector)->buf + index + 1, (vector)->buf + index, sizeof((vector)->buf[0])*((vector)->info.count - (index))); \
        (vector)->buf[(index)] = item_to_insert; \
        (vector)->info.count++; \
    } while(0)

#define vec_extend(arena, dest, src) \
    do { \
        vec_reserve(arena, dest, (src)->info.count); \
        memmove((dest)->buf + (dest)->info.count, (src)->buf, sizeof((dest)->buf[0])*(src)->info.count); \
        (dest)->info.count += (src)->info.count; \
    } while(0)

#endif // VECTOR_H
