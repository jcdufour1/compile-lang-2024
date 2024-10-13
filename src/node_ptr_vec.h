#ifndef NODE_PTR_VEC_H
#define NODE_PTR_VEC_H

#include <stddef.h>
#include "vector.h"
#include <assert.h>

#define NODE_PTR_VEC_DEFAULT_CAPACITY 1

struct Node_;

typedef struct Node_ Node;

typedef struct {
    Vec_base info;
    Node** buf;
} Node_ptr_vec;

static inline void node_ptr_vec_set_to_zero_len(Node_ptr_vec* vector) {
    vector_set_to_zero_len(vector, sizeof(vector->buf[0]));
}

static inline Node** node_ptr_vec_at_ref(Node_ptr_vec* vector, size_t idx) {
    return &vector->buf[idx];
}

static inline Node* node_ptr_vec_at(Node_ptr_vec* vector, size_t idx) {
    return vector->buf[idx];
}

static inline const Node* node_ptr_vec_at_const(const Node_ptr_vec* vector, size_t idx) {
    return vector->buf[idx];
}

static inline void node_ptr_vec_append(Node_ptr_vec* vector, Node* node_to_append) {
    vector_append(&a_main, vector, sizeof(vector->buf[0]), &node_to_append, NODE_PTR_VEC_DEFAULT_CAPACITY);
}

static inline Node* node_ptr_vec_pop(Node_ptr_vec* vector) {
    Node* node = vector->buf[vector->info.count];
    vector->info.count--;
    return node;
}

static inline void node_ptr_vec_extend(Node_ptr_vec* dest, const Node_ptr_vec src) {
    vector_extend(&a_main, dest, &src, sizeof(dest->buf[0]), NODE_PTR_VEC_DEFAULT_CAPACITY);
}

static inline void node_ptr_vec_insert(Node_ptr_vec* vector, size_t index, Node* node_to_insert) {
    vector_insert(&a_main, vector, index, sizeof(vector->buf[0]), &node_to_insert, NODE_PTR_VEC_DEFAULT_CAPACITY);
}

static inline bool node_ptr_vec_in(Node_ptr_vec* vector, const Node* num_to_find) {
    return vector_in(vector, sizeof(vector->buf[0]), &num_to_find);
}

static inline Node* node_ptr_vec_top(Node_ptr_vec* vector) {
    assert(vector->info.count > 0);
    return vector->buf[vector->info.count - 1];
}

static inline void node_ptr_assert_no_null(const Node_ptr_vec* vector) {
    for (size_t idx = 0; idx < vector->info.count; idx++) {
        assert(node_ptr_vec_at((Node_ptr_vec*)vector, idx));
    }
}

#endif // NODE_PTR_VEC_H
