#ifndef PASSES_H
#define PASSES_H

#include <node.h>
#include <do_passes.h>
#include <tokens.h>

static inline void insert_into_node_ptr_vec(
    Node_ptr_vec* block_children,
    size_t* idx_to_insert_before,
    size_t idx_to_insert_item,
    Node* node_to_insert
) {
    vec_insert(&a_main, block_children, idx_to_insert_item, node_to_insert);
    if (idx_to_insert_item <= *idx_to_insert_before) {
        (*idx_to_insert_before)++;
    }
}

void tokenize_do_test(void);

Tokens tokenize(Env* env, const Parameters params);

Node_block* parse(Env* env, const Tokens tokens);

Node_block* analysis_1(Env* env, Node_block* root);

Node_block* add_load_and_store(Env* env, Node_block* old_block);

Node_block* change_operators(Env* env, Node_block* root);

void assign_llvm_ids(Env* env);

void emit_llvm_from_tree(Env* env, const Node_block* root);

#endif // PASSES_H
