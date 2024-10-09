#ifndef PASSES_H
#define PASSES_H

#include "../node.h"

static inline bool dummy_callback(Node* curr_node, int recursion_depth) {
    (void) curr_node;
    (void) recursion_depth;
    return false;
}

bool analysis_1(Node* curr_node, int recursion_depth);

bool for_and_if_to_branch(Node* curr_node, int recursion_depth);

bool flatten_operations(Node* curr_node, int recursion_depth);

bool add_alloca(Node* curr_node, int recursion_depth);

bool add_load_and_store(Node* curr_node, int recursion_depth);

bool assign_llvm_ids(Node* curr_node, int recursion_depth);

#endif // PASSES_H
