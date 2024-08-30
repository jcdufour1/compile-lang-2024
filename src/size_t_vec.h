#ifndef SIZE_T_VEC_H
#define SIZE_T_VEC_H

#include <stddef.h>
#include "vector.h"

typedef struct {
    Vec_base info;
    size_t* buf;
} Size_t_vec;

static inline void size_t_vec_append(Size_t_vec* vector, size_t num_to_append) {
    vector_append(vector, sizeof(vector->buf[0]), &num_to_append, 1000);
}

static inline size_t size_t_vec_at(Size_t_vec* vector, size_t idx) {
    return vector->buf[idx];
}

static inline bool size_t_vec_in(Size_t_vec* vector, size_t num_to_find) {
    for (size_t idx = 0; idx < vector->info.count; idx++) {
        if (vector->buf[idx] == num_to_find) {
            return true;
        }
    }
    return false;
}

#endif // SIZE_T_VEC_H
