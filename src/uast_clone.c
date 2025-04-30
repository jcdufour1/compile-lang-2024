#include <uast_clone.h>
#include <parser_utils.h>
#include <symbol_collection_clone.h>
#include <ulang_type_clone.h>

Uast_number* uast_number_clone(const Uast_number* lit) {
    return uast_number_new(lit->pos, lit->data);
}

Uast_string* uast_string_clone(const Uast_string* lit) {
    return uast_string_new(lit->pos, lit->data, util_literal_name_new());
}

Uast_literal* uast_literal_clone(const Uast_literal* lit) {
    switch (lit->type) {
        case UAST_NUMBER:
            return uast_number_wrap(uast_number_clone(uast_number_const_unwrap(lit)));
        case UAST_STRING:
            return uast_string_wrap(uast_string_clone(uast_string_const_unwrap(lit)));
        case UAST_ENUM_LIT:
            todo();
        case UAST_CHAR:
            todo();
        case UAST_VOID:
            return uast_void_wrap(uast_void_new(uast_void_const_unwrap(lit)->pos));
    }
    unreachable("");
}

Uast_if_vec uast_if_vec_clone(Uast_if_vec vec, Scope_id new_scope) {
    Uast_if_vec new_vec = {0};
    for (size_t idx = 0; idx < vec.info.count; idx++) {
        vec_append(&a_main, &new_vec, uast_if_clone(vec_at(&vec, idx), new_scope));
    }
    return new_vec;
}

Uast_expr_vec uast_expr_vec_clone(Uast_expr_vec vec, Scope_id new_scope) {
    Uast_expr_vec new_vec = {0};
    for (size_t idx = 0; idx < vec.info.count; idx++) {
        vec_append(&a_main, &new_vec, uast_expr_clone(vec_at(&vec, idx), new_scope));
    }
    return new_vec;
}

Uast_case_vec uast_case_vec_clone(Uast_case_vec vec, Scope_id new_scope) {
    Uast_case_vec new_vec = {0};
    for (size_t idx = 0; idx < vec.info.count; idx++) {
        vec_append(&a_main, &new_vec, uast_case_clone(vec_at(&vec, idx), new_scope));
    }
    return new_vec;
}

Uast_symbol* uast_symbol_clone(const Uast_symbol* symbol, Scope_id new_scope) {
    return uast_symbol_new(symbol->pos, name_clone(symbol->name, new_scope));
}

Uast_member_access* uast_member_access_clone(const Uast_member_access* access, Scope_id new_scope) {
    return uast_member_access_new(access->pos, access->member_name, uast_expr_clone(access->callee, new_scope));
}

Uast_unary* uast_unary_clone(const Uast_unary* unary, Scope_id new_scope) {
    return uast_unary_new(unary->pos, uast_expr_clone(unary->child, new_scope), unary->token_type, ulang_type_clone(unary->lang_type, new_scope));
}

Uast_binary* uast_binary_clone(const Uast_binary* binary, Scope_id new_scope) {
    return uast_binary_new(binary->pos, uast_expr_clone(binary->lhs, new_scope), uast_expr_clone(binary->rhs, new_scope), binary->token_type);
}

Uast_index* uast_index_clone(const Uast_index* index, Scope_id new_scope) {
    return uast_index_new(index->pos, uast_expr_clone(index->index, new_scope), uast_expr_clone(index->callee, new_scope));
}

Uast_function_call* uast_function_call_clone(const Uast_function_call* fun_call, Scope_id new_scope) {
    return uast_function_call_new(
        fun_call->pos,
        uast_expr_vec_clone(fun_call->args, new_scope),
        uast_expr_clone(fun_call->callee, new_scope)
    );
}

Uast_tuple* uast_tuple_clone(const Uast_tuple* tuple, Scope_id new_scope) {
    return uast_tuple_new(
        tuple->pos,
        uast_expr_vec_clone(tuple->members, new_scope)
    );
}

Uast_operator* uast_operator_clone(const Uast_operator* oper, Scope_id new_scope) {
    switch (oper->type) {
        case UAST_UNARY:
            return uast_unary_wrap(uast_unary_clone(uast_unary_const_unwrap(oper), new_scope));
        case UAST_BINARY:
            return uast_binary_wrap(uast_binary_clone(uast_binary_const_unwrap(oper), new_scope));
    }
    unreachable("");
}

Uast_unknown* uast_unknown_clone(const Uast_unknown* unknown) {
    return uast_unknown_new(unknown->pos);
}

Uast_param* uast_param_clone(const Uast_param* param, Scope_id new_scope) {
    return uast_param_new(param->pos, uast_variable_def_clone(param->base, new_scope), param->is_optional, param->is_variadic, param->optional_default);
}

Uast_lang_def* uast_lang_def_clone(const Uast_lang_def* def) {
    (void) def;
    todo();
}

Uast_mod_alias* uast_mod_alias_clone(const Uast_mod_alias* alias, Scope_id new_scope) {
    return uast_mod_alias_new(alias->pos, name_clone(alias->name, new_scope), name_clone(alias->mod_path, new_scope));
}

Uast_expr* uast_expr_clone(const Uast_expr* expr, Scope_id new_scope) {
    switch (expr->type) {
        case UAST_OPERATOR:
            return uast_operator_wrap(uast_operator_clone(uast_operator_const_unwrap(expr), new_scope));
        case UAST_SYMBOL:
            return uast_symbol_wrap(uast_symbol_clone(uast_symbol_const_unwrap(expr), new_scope));
        case UAST_MEMBER_ACCESS:
            return uast_member_access_wrap(uast_member_access_clone(uast_member_access_const_unwrap(expr), new_scope));
        case UAST_INDEX:
            return uast_index_wrap(uast_index_clone(uast_index_const_unwrap(expr), new_scope));
        case UAST_LITERAL:
            return uast_literal_wrap(uast_literal_clone(uast_literal_const_unwrap(expr)));
        case UAST_FUNCTION_CALL:
            return uast_function_call_wrap(uast_function_call_clone(uast_function_call_const_unwrap(expr), new_scope));
        case UAST_STRUCT_LITERAL:
            todo();
        case UAST_TUPLE:
            return uast_tuple_wrap(uast_tuple_clone(uast_tuple_const_unwrap(expr), new_scope));
        case UAST_SUM_ACCESS: // TODO: remove uast_sum_access if not used
            todo();
        case UAST_UNKNOWN: // TODO: remove uast_sum_access if not used
            return uast_unknown_wrap(uast_unknown_clone(uast_unknown_const_unwrap(expr)));
        case UAST_SUM_GET_TAG: // TODO: remove uast_sum_access if not used
            todo();
        case UAST_SWITCH:
            return uast_switch_wrap(uast_switch_clone(uast_switch_const_unwrap(expr), new_scope));
        case UAST_IF_ELSE_CHAIN:
            return uast_if_else_chain_wrap(uast_if_else_chain_clone(uast_if_else_chain_const_unwrap(expr), new_scope));
        case UAST_ARRAY_LITERAL:
            todo();
    }
    unreachable("");
}

Uast_def* uast_def_clone(const Uast_def* def, Scope_id new_scope) {
    switch (def->type) {
        case UAST_GENERIC_PARAM:
            todo();
        case UAST_FUNCTION_DEF:
            todo();
        case UAST_FUNCTION_DECL:
            todo();
        case UAST_STRUCT_DEF:
            todo();
        case UAST_RAW_UNION_DEF:
            todo();
        case UAST_SUM_DEF:
            todo();
        case UAST_ENUM_DEF:
            todo();
        case UAST_PRIMITIVE_DEF:
            todo();
        case UAST_LITERAL_DEF:
            todo();
        case UAST_VARIABLE_DEF: {
            // TODO: simplify
            Uast_variable_def* new_def = uast_variable_def_clone(uast_variable_def_const_unwrap(def), new_scope);
            assert(uast_variable_def_const_unwrap(def) != new_def);
            assert(def != uast_variable_def_wrap(new_def));
            return uast_variable_def_wrap(new_def);
        }
        case UAST_POISON_DEF:
            todo();
        case UAST_MOD_ALIAS:
            return uast_mod_alias_wrap(uast_mod_alias_clone(uast_mod_alias_const_unwrap(def), new_scope));
        case UAST_IMPORT_PATH:
            todo();
        case UAST_LANG_DEF:
            return uast_lang_def_wrap(uast_lang_def_clone(uast_lang_def_const_unwrap(def)));
    }
    unreachable("");
}

Uast_for_with_cond* uast_for_with_cond_clone(const Uast_for_with_cond* lang_for, Scope_id new_scope) {
    return uast_for_with_cond_new(
        lang_for->pos,
        uast_condition_clone(lang_for->condition, new_scope),
        uast_block_clone(lang_for->body),
        lang_for->continue_label,
        lang_for->do_cont_label
    );
}

Uast_condition* uast_condition_clone(const Uast_condition* cond, Scope_id new_scope) {
    return uast_condition_new(cond->pos, uast_operator_clone(cond->child, new_scope));
}

Uast_break* uast_break_clone(const Uast_break* lang_break, Scope_id new_scope) {
    return uast_break_new(lang_break->pos, lang_break->do_break_expr, uast_expr_clone(lang_break->break_expr, new_scope));
}

Uast_continue* uast_continue_clone(const Uast_continue* cont) {
    return uast_continue_new(cont->pos);
}

Uast_assignment* uast_assignment_clone(const Uast_assignment* assign, Scope_id new_scope) {
    return uast_assignment_new(assign->pos, uast_expr_clone(assign->lhs, new_scope), uast_expr_clone(assign->rhs, new_scope));
}

Uast_return* uast_return_clone(const Uast_return* rtn, Scope_id new_scope) {
    return uast_return_new(rtn->pos, uast_expr_clone(rtn->child, new_scope), rtn->is_auto_inserted);
}

Uast_if_else_chain* uast_if_else_chain_clone(const Uast_if_else_chain* if_else, Scope_id new_scope) {
    return uast_if_else_chain_new(if_else->pos, uast_if_vec_clone(if_else->uasts, new_scope));
}

Uast_switch* uast_switch_clone(const Uast_switch* lang_switch, Scope_id new_scope) {
    return uast_switch_new(lang_switch->pos, uast_expr_clone(lang_switch->operand, new_scope), uast_case_vec_clone(lang_switch->cases, new_scope));
}

Uast_label* uast_label_clone(const Uast_label* lang_label, Scope_id new_scope) {
    (void) new_scope;
    // TODO: new_scope should be put in label
    // label may need name
    return uast_label_new(lang_label->pos, lang_label->name);
}

Uast_stmt* uast_stmt_clone(const Uast_stmt* stmt, Scope_id new_scope) {
    switch (stmt->type) {
        case UAST_EXPR:
            return uast_expr_wrap(uast_expr_clone(uast_expr_const_unwrap(stmt), new_scope));
        case UAST_BLOCK:
            return uast_block_wrap(uast_block_clone(uast_block_const_unwrap(stmt)));
        case UAST_DEF:
            return uast_def_wrap(uast_def_clone(uast_def_const_unwrap(stmt), new_scope));
        case UAST_FOR_WITH_COND:
            return uast_for_with_cond_wrap(uast_for_with_cond_clone(uast_for_with_cond_const_unwrap(stmt), new_scope));
        case UAST_BREAK:
            return uast_break_wrap(uast_break_clone(uast_break_const_unwrap(stmt), new_scope));
        case UAST_CONTINUE:
            return uast_continue_wrap(uast_continue_clone(uast_continue_const_unwrap(stmt)));
        case UAST_ASSIGNMENT:
            return uast_assignment_wrap(uast_assignment_clone(uast_assignment_const_unwrap(stmt), new_scope));
        case UAST_RETURN:
            return uast_return_wrap(uast_return_clone(uast_return_const_unwrap(stmt), new_scope));
        case UAST_LABEL:
            return uast_label_wrap(uast_label_clone(uast_label_const_unwrap(stmt), new_scope));
    }
    unreachable("");
}

Uast_case* uast_case_clone(const Uast_case* lang_case, Scope_id new_scope) {
    return uast_case_new(lang_case->pos, lang_case->is_default, uast_expr_clone(lang_case->expr, new_scope), uast_stmt_clone(lang_case->if_true, new_scope));
}

Uast_variable_def* uast_variable_def_clone(const Uast_variable_def* def, Scope_id new_scope) {
    return uast_variable_def_new(def->pos, ulang_type_clone(def->lang_type, new_scope), def->name);
}

Uast_block* uast_block_clone(const Uast_block* block) {
    Uast_stmt_vec new_children = {0};
    Scope_id new_scope = scope_id_clone(block->scope_id);
    for (size_t idx = 0; idx < block->children.info.count; idx++) {
        vec_append(&a_main, &new_children, uast_stmt_clone(vec_at(&block->children, idx), new_scope));
    }
    return uast_block_new(block->pos, new_children, block->pos_end, new_scope);
}

Uast_if* uast_if_clone(const Uast_if* lang_if, Scope_id new_scope) {
    return uast_if_new(lang_if->pos, uast_condition_clone(lang_if->condition, new_scope), uast_block_clone(lang_if->body));
}
