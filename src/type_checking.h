#ifndef TYPE_CHECKING_H
#define TYPE_CHECKING_H

#include <stddef.h>
#include <util.h>
#include <parser_utils.h>

bool try_set_assignment_types(Env* env, Tast_assignment** new_assign, Uast_assignment* assignment);

// returns false if unsuccessful
bool try_set_expr_types(Env* env, Tast_expr** new_tast, Uast_expr* expr);

bool try_set_binary_types_finish(
    Env* env,
    Tast_expr** new_tast,
    Tast_expr* new_lhs,
    Tast_expr* new_rhs,
    Pos oper_pos,
    TOKEN_TYPE oper_token_type
);

// returns false if unsuccessful
bool try_set_binary_types(Env* env, Tast_expr** new_tast, Uast_binary* operator);

bool try_set_block_types(Env* env, Tast_block** new_tast, Uast_block* tast, bool is_directly_in_fun_def);

bool try_set_stmt_types(Env* env, Tast_stmt** new_stmt, Uast_stmt* stmt);

bool try_set_uast_types(Env* env, Tast** new_tast, Uast* tast);

// returns false if unsuccessful
bool try_set_binary_operand_types(Tast_expr* operand);

bool try_set_unary_types_finish(
    Env* env,
    Tast_expr** new_tast,
    Tast_expr* new_child,
    Pos unary_pos,
    TOKEN_TYPE unary_token_type,
    Lang_type old_lang_type
);

bool try_set_unary_types(Env* env, Tast_expr** new_tast, Uast_unary* unary);

bool try_set_tuple_assignment_types(
    Env* env,
    Tast_tuple** new_tast,
    Lang_type dest_lang_type,
    Uast_tuple* tuple
);

bool try_set_struct_literal_assignment_types(
    Env* env,
    Tast_stmt** new_tast,
    Lang_type dest_lang_type,
    Uast_struct_literal* struct_literal,
    Pos assign_pos
);

Tast_literal* try_set_literal_types(Uast_literal* literal);

bool try_set_function_call_types(Env* env, Tast_expr** new_call, Uast_function_call* fun_call);

bool try_set_member_access_types(Env* env, Tast_stmt** new_tast, Uast_member_access* access);

bool try_set_function_def_types(
    Env* env,
    Tast_function_def** new_tast,
    Uast_function_def* decl
);

bool try_set_function_decl_types(
    Env* env,
    Tast_function_decl** new_tast,
    Uast_function_decl* decl,
    bool add_to_sym_tbl
);

bool try_set_sum_def_types(Env* env, Tast_sum_def** new_tast, Uast_sum_def* tast);

bool try_set_tuple_types(Env* env, Tast_tuple** new_tuple, Uast_tuple* tuple);

bool try_set_sum_access_types(Env* env, Tast_sum_access** new_sum_access, Uast_sum_access* sum_access);

bool try_set_index_untyped_types(Env* env, Tast_stmt** new_tast, Uast_index* index);

bool try_set_function_params_types(
    Env* env,
    Tast_function_params** new_tast,
    Uast_function_params* def,
    bool add_to_sym_tbl
);

bool try_types_set_lang_type(
    Env* env,
    Tast_lang_type** new_tast,
    Uast_lang_type* def
);

bool try_set_symbol_type(const Env* env, Tast_expr** new_tast, Uast_symbol* sym_untyped);

bool try_set_primitive_def_types(Env* env, Tast_primitive_def** new_tast, Uast_primitive_def* tast);

bool try_set_literal_def_types(Env* env, Tast_literal_def** new_tast, Uast_literal_def* tast);

#endif // TYPE_CHECKING_H
