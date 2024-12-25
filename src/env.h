#ifndef ENV_H
#define ENV_H

#include "symbol_table_struct.h"
#include <node_forward_decl.h>
#include <llvm_forward_decl.h>

typedef struct {
    Vec_base info;
    Symbol_collection** buf;
} Sym_coll_vec;

typedef struct Env_ {
    Sym_coll_vec ancesters; // index 0 is the root of the tree
                            // index len - 1 is the current node
    Llvm_vec llvm_ancesters; // index 0 is the root of the tree
                                 // index len - 1 is the current node
    Node_def_vec defered_symbols_to_add;
    Llvm_vec defered_allocas_to_add;
    Symbol_table global_literals; // this is populated during add_load_and_store pass
    Symbol_table primitives;
    int recursion_depth;
    Str_view file_text;

    Str_view name_parent_function; // length is zero if no parent function exists

    Str_view label_if_break;
    Str_view label_if_continue;
} Env;

#endif // ENV_H
