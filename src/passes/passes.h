#ifndef PASSES_H
#define PASSES_H

#include "../node.h"
#include "../do_passes.h"

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

void analysis_1(Env* env);

void for_and_if_to_branch(Env* env);

void flatten_operations(Env* env);

void add_alloca(Env* env);

void add_load_and_store(Env* env);

void change_operators(Env* env);

void assign_llvm_ids(Env* env);

#endif // PASSES_H
