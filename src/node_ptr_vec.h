#ifndef NODE_PTR_VEC_H
#define NODE_PTR_VEC_H

#include <stddef.h>
#include "vector.h"
#include "node.h"

typedef struct {
    Vec_base info;
    Node** buf;
} Node_ptr_vec;

static inline void node_ptr_vec_append(Node_ptr_vec* vector, Node* num_to_append) {
    vector_append(vector, sizeof(vector->buf[0]), &num_to_append, 1000);
}

static inline Node* node_ptr_vec_at(Node_ptr_vec* vector, size_t idx) {
    return vector->buf[idx];
}

static inline bool node_ptr_vec_in(Node_ptr_vec* vector, Node* num_to_find) {
    return vector_in(vector, sizeof(vector->buf[0]), &num_to_find);
}

#endif // NODE_PTR_VEC_H
