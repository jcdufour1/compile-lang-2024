#ifndef STRING_VEC_H
#define STRING_VEC_H

#include "vector.h"
#include "newstring.h"

#define STRING_VEC_DEFAULT_CAPACITY 8

typedef struct {
    Vec_base info;
    String* buf;
} String_vec;

static inline void string_vec_reserve(Arena* arena, String_vec* str, size_t minimum_count_empty_slots) {
    vector_reserve(arena, str, sizeof(str->buf[0]), minimum_count_empty_slots, STRING_VEC_DEFAULT_CAPACITY);
}

static inline void string_vec_append(Arena* arena, String_vec* str, String ch) {
    vector_append(arena, str, sizeof(str->buf[0]), &ch, STRING_VEC_DEFAULT_CAPACITY);
}

#endif // STRING_VEC_H
