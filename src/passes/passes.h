#ifndef PASSES_H
#define PASSES_H

#include "../node.h"

static inline void insert_into_node_ptr_vec(
    Node_ptr_vec* block_children,
    size_t* idx_to_insert_before,
    size_t idx_to_insert_item,
    Node* node_to_insert
) {
    for (size_t idx = 0; idx < block_children->info.count; idx++) {
        node_print(vec_at(block_children, idx));
    }
    log(LOG_DEBUG, "sizeof: %zu\n", sizeof(block_children->buf[0]));
    vec_insert(&a_main, block_children, idx_to_insert_item, node_to_insert);
    for (size_t idx = 0; idx < block_children->info.count; idx++) {
        node_print(vec_at(block_children, idx));
    }
    if (idx_to_insert_item <= *idx_to_insert_before) {
        (*idx_to_insert_before)++;
    }
}

bool analysis_1(Node* curr_node, int recursion_depth);

bool for_and_if_to_branch(Node* curr_node, int recursion_depth);

bool add_alloca(Node* curr_node, int recursion_depth);

bool add_load_and_store(Node* curr_node, int recursion_depth);

bool change_operators(Node* curr_node, int recursion_depth);

bool assign_llvm_ids(Node* curr_node, int recursion_depth);

#endif // PASSES_H
