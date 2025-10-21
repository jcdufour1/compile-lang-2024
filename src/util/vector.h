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

#define vec_reserve_no_arena(vector, min_count_empty_slots) \
    do { \
        size_t original_capacity = (vector)->info.capacity; \
        size_t needs_resizing = false; \
        while ((vector)->info.capacity < (vector)->info.count + (min_count_empty_slots)) { \
            if ((vector)->info.capacity < 1) { \
                (vector)->info.capacity = 2; \
            } else { \
                (vector)->info.capacity *= 2; \
            } \
            needs_resizing = true; \
        } \
        if (original_capacity == 0) { \
            (vector)->buf = malloc(sizeof((vector)->buf[0])*(vector)->info.capacity)); \
        } else if (needs_resizing) { \
            (vector)->buf = realloc((vector)->buf, sizeof((vector)->buf[0])*(vector)->info.capacity)); \
        } \
    } while(0)

#define vec_append_no_arena(vector, item_to_append) \
    do { \
        vec_reserve_no_arena(vector, 1); \
        (vector)->buf[(vector)->info.count++] = (item_to_append); \
    } while(0)

#define vec_reserve(arena, vector, min_count_empty_slots) \
    do { \
        size_t original_capacity = (vector)->info.capacity; \
        size_t needs_resizing = false; \
        while ((vector)->info.capacity < (vector)->info.count + (min_count_empty_slots)) { \
            if ((vector)->info.capacity < 1) { \
                (vector)->info.capacity = 2; \
            } else { \
                (vector)->info.capacity *= 2; \
            } \
            needs_resizing = true; \
        } \
        if (original_capacity == 0) { \
            (vector)->buf = arena_alloc(arena, sizeof((vector)->buf[0])*(vector)->info.capacity); \
        } else if (needs_resizing) { \
            (vector)->buf = arena_realloc(arena, (vector)->buf, sizeof((vector)->buf[0])*original_capacity, sizeof((vector)->buf[0])*(vector)->info.capacity); \
        } \
    } while(0)

#define vec_append(arena, vector, item_to_append) \
    do { \
        vec_reserve(arena, vector, 1); \
        (vector)->buf[(vector)->info.count++] = (item_to_append); \
    } while(0)

#define vec_reset(vector) \
    do { \
        (vector)->info.count = 0; \
    } while(0)

#define vec_at(vector, index) \
    (unwrap((vector)->info.count > (index) && "out of bounds"), (vector)->buf[(index)])

// TODO: make `vec_at_const` the new at function everywhere
#define vec_at_const(vector, index) \
    (unwrap((vector).info.count > (index) && "out of bounds"), (vector).buf[(index)])

#define vec_at_ref(vector, index) \
    (unwrap((vector)->info.count > (index) && "out of bounds"), &(vector)->buf[(index)])

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

#define vec_top(vector) \
    (unwrap((vector)->info.count > 0 && "out of bounds"), vec_at((vector), (vector)->info.count - 1))

#define vec_top_ref(vector) \
    (unwrap((vector)->info.count > 0 && "out of bounds"), vec_at_ref((vector), (vector)->info.count - 1))

#define vec_pop(vector) \
    (unwrap((vector)->info.count > 0 && "out of bounds"), (vector)->info.count--, (vector)->buf[((vector)->info.count)])

// TODO: remove this macro (use vec_pop instead)
#define vec_rem_last(vector) \
    do { \
        (vector)->info.count--; \
    } while(0)

// NOTE: `}}` must be used to end foreach body instead of `}`
#define vec_foreach(idx_name, Item_type, item_name, vector) \
    { \
        Item_type item_name = {0}; \
        for (size_t idx_name = 0; item_name = (idx_name < (vector).info.count) ? vec_at(&vector, idx_name) : ((Item_type) {0}), idx_name < (vector).info.count; idx_name++) 

// NOTE: `}}` must be used to end foreach body instead of `}`
#define vec_foreach_ref(idx_name, Item_type, item_name, vector) \
    { \
        Item_type* item_name = NULL; \
        for (size_t idx_name = 0; item_name = (idx_name < (vector).info.count) ? vec_at_ref(&vector, idx_name) : NULL, idx_name < (vector).info.count; idx_name++) 

#endif // VECTOR_H
