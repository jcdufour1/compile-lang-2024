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

void analysis_1(Env* env);

void for_and_if_to_branch(Env* env);

void flatten_operations(Env* env);

void add_alloca(Env* env);

Node_block* add_load_and_store(Env* env, const Node_block* old_block);

void change_operators(Env* env);

void assign_llvm_ids(Env* env);

void emit_llvm_from_tree(Env* env, const Node_block* root);

#endif // PASSES_H
