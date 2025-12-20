#ifndef CSTR_VEC_H
#define CSTR_VEC_H

#include <darr.h>

typedef struct {
    Vec_base info;
    const char** buf;
} Cstr_darr;

// note: NULL terminator automatically added to the end of the result
static inline char** cstr_darr_to_c_cstr_darr(Arena* arena, Cstr_darr darr) {
    char** buf = arena_alloc(arena, sizeof(*buf)*(darr.info.count + 1));
    for (size_t idx = 0; idx < darr.info.count; idx++) {
        buf[idx] = arena_strdup_mut(arena, darr_at(darr, idx));
    }
    return buf;
}

#endif // CSTR_VEC_H
