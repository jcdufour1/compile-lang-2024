#ifndef PASSES_H
#define PASSES_H

#include <tast.h>
#include <llvm.h>
#include <do_passes.h>
#include <tokens.h>

static inline void insert_into_tast_vec(
    Tast_vec* block_children,
    size_t* idx_to_insert_before,
    size_t idx_to_insert_item,
    Tast* tast_to_insert
) {
    vec_insert(&a_main, block_children, idx_to_insert_item, tast_to_insert);
    if (idx_to_insert_item <= *idx_to_insert_before) {
        (*idx_to_insert_before)++;
    }
}

void tokenize_do_test(void);

bool tokenize(Tokens* result, Env* env, Str_view file_path);

bool parse_file(Uast_block** block, Env* env, Str_view file_path, bool do_new_sym_coll);

Tast_block* analysis_1(Env* env, Uast_block* root);

Tast_block* change_operators(Env* env, Tast_block* root);

Tast_block* remove_tuples(Env* env, Tast_block* root);

Llvm_block* add_load_and_store(Env* env, Tast_block* old_block);

Llvm_block* assign_llvm_ids(Env* env, Llvm_block* root);

void emit_llvm_from_tree(Env* env, const Llvm_block* root);

#endif // PASSES_H
