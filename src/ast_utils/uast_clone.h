#ifndef UAST_CLONE_H
#define UAST_CLONE_H

#include <uast.h>

Uast_char* uast_char_clone(const Uast_char* lit);

Uast_literal* uast_literal_clone(const Uast_literal* lit);

Uast_expr* uast_expr_clone(const Uast_expr* expr, Scope_id new_scope, Pos dest_pos);

Uast_stmt* uast_stmt_clone(const Uast_stmt* stmt, Scope_id new_scope, Pos dest_pos);

Uast_variable_def* uast_variable_def_clone(const Uast_variable_def* def, Scope_id new_scope);

Uast_param* uast_param_clone(const Uast_param* param, Scope_id new_scope);

Uast_block* uast_block_clone(const Uast_block* block, Scope_id parent, Pos dest_pos);

Uast_def* uast_def_clone(const Uast_def* def, Scope_id new_scope);

Uast_case* uast_case_clone(const Uast_case* lang_case, Scope_id new_scope, Pos dest_pos);

Uast_condition* uast_condition_clone(const Uast_condition* cond, Scope_id new_scope, Pos dest_pos);

Uast_if* uast_if_clone(const Uast_if* lang_if, Scope_id new_scope, Pos dest_pos);

Uast_switch* uast_switch_clone(const Uast_switch* lang_switch, Scope_id new_scope, Pos dest_pos);

Uast_if_else_chain* uast_if_else_chain_clone(const Uast_if_else_chain* if_else, Scope_id new_scope, Pos dest_pos);

Uast_function_decl* uast_function_decl_clone(const Uast_function_decl* decl, Scope_id new_scope);

Uast_generic_param* uast_generic_param_clone(const Uast_generic_param* param, Scope_id new_scope);

Uast_function_params* uast_function_params_clone(const Uast_function_params* params, Scope_id scope_id);

#endif // UAST_CLONE_H
