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

#define darr_reserve(arena, darr, min_count_empty_slots) \
    do { \
        size_t original_capacity = (darr)->info.capacity; \
        bool needs_resizing = false; \
        while ((darr)->info.capacity < (darr)->info.count + (min_count_empty_slots)) { \
            if ((darr)->info.capacity < 1) { \
                (darr)->info.capacity = 2; \
            } else { \
                (darr)->info.capacity *= 2; \
            } \
            needs_resizing = true; \
        } \
        if (needs_resizing && original_capacity == 0) { \
            (darr)->buf = arena_alloc(arena, sizeof((darr)->buf[0])*(darr)->info.capacity); \
        } else if (needs_resizing) { \
            (darr)->buf = arena_realloc(arena, (darr)->buf, sizeof((darr)->buf[0])*original_capacity, sizeof((darr)->buf[0])*(darr)->info.capacity); \
        } \
    } while(0)

#define darr_append(arena, darr, item_to_append) \
    do { \
        darr_reserve(arena, darr, 1); \
        (darr)->buf[(darr)->info.count++] = (item_to_append); \
    } while(0)

#define darr_reset(darr) \
    do { \
        (darr)->info.count = 0; \
    } while(0)

#define darr_at(darr, index) \
    (unwrap((darr).info.count > (index) && "out of bounds"), (darr).buf[(index)])

#define darr_at_ref(darr, index) \
    (unwrap((darr)->info.count > (index) && "out of bounds"), &(darr)->buf[(index)])

#define darr_insert(arena, darr, index, item_to_insert) \
    do { \
        (unwrap((darr)->info.count >= (index) && "out of bounds"), &(darr)->buf[(index)]); \
        darr_reserve(arena, darr, 1); \
        memmove((darr)->buf + index + 1, (darr)->buf + index, sizeof((darr)->buf[0])*((darr)->info.count - (index))); \
        (darr)->buf[(index)] = item_to_insert; \
        (darr)->info.count++; \
    } while(0)

#define darr_extend(arena, dest, src) \
    do { \
        darr_reserve(arena, dest, (src)->info.count); \
        if (sizeof((dest)->buf[0])*(src)->info.count > 0) { \
            unwrap(dest); \
            memmove((dest)->buf + (dest)->info.count, (src)->buf, sizeof((dest)->buf[0])*(src)->info.count); \
        } \
        (dest)->info.count += (src)->info.count; \
    } while(0)

#define darr_first(darr) \
    darr_at_ref((darr), 0)

#define darr_first_ref(darr) \
    darr_at_ref((darr), 0)

#define darr_last(darr) \
    (unwrap((darr).info.count > 0 && "out of bounds"), darr_at((darr), (darr).info.count - 1))

#define darr_top(darr) \
    darr_last(darr)

#define darr_last_ref(darr) \
    (unwrap((darr)->info.count > 0 && "out of bounds"), darr_at_ref((darr), (darr)->info.count - 1))

#define darr_top_ref(darr) \
    darr_last_ref(darr)

#define darr_pop(darr) \
    (unwrap((darr)->info.count > 0 && "out of bounds"), (darr)->info.count--, (darr)->buf[((darr)->info.count)])

#define darr_foreach(idx_name, Item_type, item_name, darr) \
    Item_type item_name = {0}; \
    for (size_t idx_name = 0; item_name = (idx_name < (darr).info.count) ? darr_at(darr, idx_name) : ((Item_type) {0}), idx_name < (darr).info.count; idx_name++) 

#define darr_foreach_ref(idx_name, Item_type, item_name, darr) \
    Item_type* item_name = NULL; \
    for (size_t idx_name = 0; item_name = (idx_name < (darr).info.count) ? darr_at_ref(&darr, idx_name) : NULL, idx_name < (darr).info.count; idx_name++) 

#define darr_swap(darr, Item_type, idx_lhs, idx_rhs) \
    do { \
        Item_type temp = darr_at(*(darr), idx_lhs); \
        *darr_at_ref(darr, idx_lhs) = darr_at(*(darr), idx_rhs); \
        *darr_at_ref(darr, idx_rhs) = temp; \
    } while(0)

#define darr_dump(log_level, darr, item_print_fn) \
    { \
        log(log_level, "["); \
        for (size_t idx = 0; idx < (darr).info.count; idx++) { \
            if (idx > 0) { \
                log_internal_ex(stderr, log_level, false, "", 0, 0, ", "); \
            } \
            log_internal_ex(stderr, log_level, false, "", 0, 0, FMT, item_print_fn(darr_at(darr, idx))); \
        } \
        log_internal_ex(stderr, log_level, false, "", 0, 0, "]\n"); \
    }

#endif // VECTOR_H
