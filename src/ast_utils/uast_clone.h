#ifndef UAST_CLONE_H
#define UAST_CLONE_H

#include <uast.h>

Uast_literal* uast_literal_clone(const Uast_literal* lit);

Uast_expr* uast_expr_clone(const Uast_expr* expr, bool use_new_scope, Scope_id new_scope, Pos dest_pos);

Uast_stmt* uast_stmt_clone(const Uast_stmt* stmt, bool use_new_scope, Scope_id new_scope, Pos dest_pos);

Uast_variable_def* uast_variable_def_clone(const Uast_variable_def* def, bool use_new_scope, Scope_id new_scope);

Uast_param* uast_param_clone(const Uast_param* param, bool use_new_scope, Scope_id new_scope);

Uast_block* uast_block_clone(const Uast_block* block, bool use_new_scope, Scope_id parent, Pos dest_pos);

Uast_def* uast_def_clone(const Uast_def* def, bool use_new_scope, Scope_id new_scope);

Uast_case* uast_case_clone(const Uast_case* lang_case, bool use_new_scope, Scope_id new_scope, Pos dest_pos);

Uast_condition* uast_condition_clone(const Uast_condition* cond, bool use_new_scope, Scope_id new_scope, Pos dest_pos);

Uast_if* uast_if_clone(const Uast_if* lang_if, bool use_new_scope, Scope_id new_scope, Pos dest_pos);

Uast_switch* uast_switch_clone(const Uast_switch* lang_switch, bool use_new_scope, Scope_id new_scope, Pos dest_pos);

Uast_if_else_chain* uast_if_else_chain_clone(const Uast_if_else_chain* if_else, bool use_new_scope, Scope_id new_scope, Pos dest_pos);

Uast_function_decl* uast_function_decl_clone(const Uast_function_decl* decl, bool use_new_scope, Scope_id new_scope);

Uast_generic_param* uast_generic_param_clone(const Uast_generic_param* param, bool use_new_scope, Scope_id new_scope);

Uast_function_params* uast_function_params_clone(const Uast_function_params* params, bool use_new_scope, Scope_id scope_id);

Uast_array_literal* uast_array_literal_clone(const Uast_array_literal* if_else, bool use_new_scope, Scope_id new_scope, Pos dest_pos);

Uast_function_call* uast_function_call_clone(const Uast_function_call* fun_call, bool use_new_scope, Scope_id new_scope, Pos dest_pos);

Uast_enum_get_tag* uast_enum_get_tag_clone(
    const Uast_enum_get_tag* get_tag,
    bool use_new_scope,
    Scope_id new_scope,
    Pos dest_pos
);

Uast_orelse* uast_orelse_clone(const Uast_orelse* orelse, bool use_new_scope, Scope_id new_scope, Pos dest_pos);

Uast_fn* uast_fn_clone(const Uast_fn* fn, bool use_new_scope, Scope_id new_scope, Pos dest_pos);

Uast_expr_removed* uast_expr_removed_clone(const Uast_expr_removed* removed, bool use_new_scope, Scope_id new_scope, Pos dest_pos);

#endif // UAST_CLONE_H
