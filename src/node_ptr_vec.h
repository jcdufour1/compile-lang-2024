#ifndef NODE_PTR_VEC_H
#define NODE_PTR_VEC_H

#include <stddef.h>
#include "vector.h"
#include "node.h"

#define NODE_PTR_VEC_DEFAULT_CAPACITY 1

typedef struct {
    Vec_base info;
    Node** buf;
} Node_ptr_vec;

static inline void node_ptr_vec_set_to_zero_len(Node_ptr_vec* vector) {
    vector_set_to_zero_len(vector, sizeof(vector->buf[0]));
}

static inline void node_ptr_vec_append(Node_ptr_vec* vector, const Node* num_to_append) {
    vector_append(vector, sizeof(vector->buf[0]), &num_to_append, NODE_PTR_VEC_DEFAULT_CAPACITY);
}

static inline Node* node_ptr_vec_at(Node_ptr_vec* vector, size_t idx) {
    return vector->buf[idx];
}

static inline bool node_ptr_vec_in(Node_ptr_vec* vector, const Node* num_to_find) {
    return vector_in(vector, sizeof(vector->buf[0]), &num_to_find);
}

#endif // NODE_PTR_VEC_H
