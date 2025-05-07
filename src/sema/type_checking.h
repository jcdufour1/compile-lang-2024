#ifndef TYPE_CHECKING_H
#define TYPE_CHECKING_H

#include <stddef.h>
#include <util.h>
#include <parser_utils.h>

typedef enum {
    STMT_OK,
    STMT_NO_STMT, // new_tast is invalid and should not be added to block
    STMT_ERROR,
} STMT_STATUS;

bool try_set_assignment_types(Tast_assignment** new_assign, Uast_assignment* assignment);

// returns false if unsuccessful
bool try_set_expr_types(Tast_expr** new_tast, Uast_expr* expr);

bool try_set_binary_types_finish(
     
    Tast_expr** new_tast,
    Tast_expr* new_lhs,
    Tast_expr* new_rhs,
    Pos oper_pos,
    BINARY_TYPE oper_token_type
);

// returns false if unsuccessful
bool try_set_binary_types(Tast_expr** new_tast, Uast_binary* operator);

bool try_set_block_types(Tast_block** new_tast, Uast_block* tast, bool is_directly_in_fun_def);

STMT_STATUS try_set_stmt_types(Tast_stmt** new_stmt, Uast_stmt* stmt, bool is_at_top_level);

bool try_set_uast_types(Tast** new_tast, Uast* tast);

// returns false if unsuccessful
bool try_set_binary_operand_types(Tast_expr* operand);

bool try_set_unary_types_finish(
     
    Tast_expr** new_tast,
    Tast_expr* new_child,
    Pos unary_pos,
    UNARY_TYPE unary_token_type,
    Lang_type old_lang_type
);

bool try_set_unary_types(Tast_expr** new_tast, Uast_unary* unary);

bool try_set_tuple_assignment_types(
     
    Tast_tuple** new_tast,
    Lang_type dest_lang_type,
    Uast_tuple* tuple
);

bool try_set_struct_literal_types(
     
    Tast_stmt** new_tast,
    Lang_type dest_lang_type,
    Uast_struct_literal* lit,
    Pos assign_pos
);

bool try_set_array_literal_types(
     
    Tast_stmt** new_tast,
    Lang_type dest_lang_type,
    Uast_array_literal* lit,
    Pos assign_pos
);

Tast_literal* try_set_literal_types(Uast_literal* literal);

bool try_set_function_call_types(Tast_expr** new_call, Uast_function_call* fun_call);

bool try_set_member_access_types(Tast_stmt** new_tast, Uast_member_access* access);

bool try_set_function_def_types(
    Uast_function_def* decl
);

bool try_set_function_decl_types(
     
    Tast_function_decl** new_decl,
    Uast_function_decl* decl,
    bool add_to_sym_tbl
);

bool try_set_tuple_types(Tast_tuple** new_tuple, Uast_tuple* tuple);

bool try_set_sum_access_types(Tast_sum_access** new_sum_access, Uast_sum_access* sum_access);

bool try_set_sum_get_tag_types(Tast_sum_get_tag** new_access, Uast_sum_get_tag* access);

bool try_set_index_untyped_types(Tast_stmt** new_tast, Uast_index* index);

bool try_set_function_params_types(
     
    Tast_function_params** new_tast,
    Uast_function_params* def,
    bool add_to_sym_tbl
);

bool try_set_symbol_types(Tast_expr** new_tast, Uast_symbol* sym_untyped);

bool try_set_primitive_def_types(Uast_primitive_def* tast);

bool try_set_import_path_types(Tast_block** new_tast, Uast_import_path* tast);

bool try_set_module_alias_types(Tast_block** new_tast, Uast_mod_alias* tast);

bool try_set_switch_types(Tast_if_else_chain** new_tast, const Uast_switch* lang_switch);

bool try_set_if_else_chain(Tast_if_else_chain** new_tast, Uast_if_else_chain* if_else);

bool try_set_break_types(Tast_break** new_tast, Uast_break* lang_break);

bool try_set_lang_def_types(Uast_lang_def* tast);

bool try_set_types(Tast_block** new_tast, Uast_block* block);

#endif // TYPE_CHECKING_H
