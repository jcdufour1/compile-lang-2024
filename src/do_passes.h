#ifndef PASSES_H
#define PASSES_H

#include <tast.h>
#include <ir.h>
#include <do_passes.h>
#include <token_vec.h>
#include <parameters.h>

void tokenize_do_test(void);

bool tokenize(Token_vec* result, Strv file_path);

bool parse(Uast_block** block, Strv file_path);

Tast_block* change_operators(Tast_block* root);

Tast_block* remove_tuples(Tast_block* root);

void add_load_and_store(void);

void construct_cfgs(void);

void remove_void_assigns(void);

void check_uninitialized(void);

Ir_block* assign_ir_ids(Ir_block* root);

void emit_llvm_from_tree(const Ir_block* root);

void emit_c_from_tree(void);

#endif // PASSES_H
