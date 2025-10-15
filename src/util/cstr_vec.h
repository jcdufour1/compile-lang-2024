#ifndef CSTR_VEC_H
#define CSTR_VEC_H

#include <vector.h>

typedef struct {
    Vec_base info;
    const char** buf;
} Cstr_vec;

// note: NULL terminator automatically added to the end of the result
static inline char** cstr_vec_to_c_cstr_vec(Arena* arena, Cstr_vec vec) {
    char** buf = arena_alloc(arena, sizeof(*buf)*(vec.info.count + 1));
    for (size_t idx = 0; idx < vec.info.count; idx++) {
        buf[idx] = arena_strdup_mut(arena, vec_at(vec, idx));
    }
    return buf;
}

#endif // CSTR_VEC_H
