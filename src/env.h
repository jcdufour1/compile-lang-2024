#ifndef ENV_H
#define ENV_H

#include "symbol_table_struct.h"
#include <tast_forward_decl.h>
#include <llvm_forward_decl.h>

typedef struct {
    Vec_base info;
    Symbol_collection** buf;
} Sym_coll_vec;

// only used in type_checking.c
typedef enum {
    PARENT_OF_NONE = 0,
    PARENT_OF_CASE,
    PARENT_OF_ASSIGN_RHS,
    PARENT_OF_RETURN,
    PARENT_OF_YIELD,
    PARENT_OF_IF,
} PARENT_OF;

typedef struct Env_ {
    Sym_coll_vec ancesters; // index 0 is the root of the tree
                            // index len - 1 is the current tast
    Llvm_vec llvm_ancesters; // index 0 is the root of the tree
                                 // index len - 1 is the current tast
    Uast_def_vec udefered_symbols_to_add;
    Tast_def_vec defered_symbols_to_add;
    Llvm_vec defered_allocas_to_add;
    Usymbol_table primitives;
    int recursion_depth;
    Str_view file_text;

    bool type_checking_is_in_struct_base_def;

    Tast_variable_def_vec sum_case_vars;

    Str_view name_parent_function; // length is zero if no parent function exists
    Lang_type yield_type;

    Lang_type rm_tuple_parent_fn_lang_type;

    Uast_stmt_vec switch_case_defer_add_if_true;
    Uast_stmt_vec switch_case_defer_add_sum_case_part;

    Str_view struct_rtn_name_parent_function;

    Lang_type lhs_lang_type;
    PARENT_OF parent_of;
    Uast_expr* parent_of_operand;

    Str_view load_yield_symbol_name;
    Str_view label_if_break;
    Str_view label_if_continue;
} Env;

#endif // ENV_H
