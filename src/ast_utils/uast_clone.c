#include <uast_clone.h>
#include <parser_utils.h>
#include <symbol_collection_clone.h>
#include <ulang_type_clone.h>
#include <symbol_log.h>

// TODO: cloning symbol may not always work correctly with nested scopes?
Uast_int* uast_int_clone(const Uast_int* lit) {
    return uast_int_new(lit->pos, lit->data);
}

Uast_float* uast_float_clone(const Uast_float* lit) {
    return uast_float_new(lit->pos, lit->data);
}

Uast_string* uast_string_clone(const Uast_string* lit) {
    return uast_string_new(lit->pos, lit->data);
}

Uast_char* uast_char_clone(const Uast_char* lit) {
    return uast_char_new(lit->pos, lit->data);
}

Uast_literal* uast_literal_clone(const Uast_literal* lit) {
    switch (lit->type) {
        case UAST_INT:
            return uast_int_wrap(uast_int_clone(uast_int_const_unwrap(lit)));
        case UAST_FLOAT:
            return uast_float_wrap(uast_float_clone(uast_float_const_unwrap(lit)));
        case UAST_STRING:
            return uast_string_wrap(uast_string_clone(uast_string_const_unwrap(lit)));
        case UAST_CHAR:
            return uast_char_wrap(uast_char_clone(uast_char_const_unwrap(lit)));
        case UAST_VOID:
            return uast_void_wrap(uast_void_new(uast_void_const_unwrap(lit)->pos));
    }
    unreachable("");
}

Uast_generic_param_vec uast_generic_param_vec_clone(Uast_generic_param_vec vec, bool use_new_scope, Scope_id new_scope) {
    Uast_generic_param_vec new_vec = {0};
    for (size_t idx = 0; idx < vec.info.count; idx++) {
        vec_append(&a_main, &new_vec, uast_generic_param_clone(vec_at(&vec, idx), use_new_scope, new_scope));
    }
    return new_vec;
}

Uast_param_vec uast_param_vec_clone(Uast_param_vec vec, bool use_new_scope, Scope_id new_scope) {
    Uast_param_vec new_vec = {0};
    for (size_t idx = 0; idx < vec.info.count; idx++) {
        vec_append(&a_main, &new_vec, uast_param_clone(vec_at(&vec, idx), use_new_scope, new_scope));
    }
    return new_vec;
}

Uast_if_vec uast_if_vec_clone(Uast_if_vec vec, bool use_new_scope, Scope_id new_scope, Pos dest_pos) {
    Uast_if_vec new_vec = {0};
    for (size_t idx = 0; idx < vec.info.count; idx++) {
        vec_append(&a_main, &new_vec, uast_if_clone(vec_at(&vec, idx), use_new_scope, new_scope, dest_pos));
    }
    return new_vec;
}

Uast_expr_vec uast_expr_vec_clone(Uast_expr_vec vec, bool use_new_scope, Scope_id new_scope, Pos dest_pos) {
    Uast_expr_vec new_vec = {0};
    for (size_t idx = 0; idx < vec.info.count; idx++) {
        vec_append(&a_main, &new_vec, uast_expr_clone(vec_at(&vec, idx), use_new_scope, new_scope, dest_pos));
    }
    return new_vec;
}

Uast_case_vec uast_case_vec_clone(Uast_case_vec vec, bool use_new_scope, Scope_id new_scope, Pos dest_pos) {
    Uast_case_vec new_vec = {0};
    for (size_t idx = 0; idx < vec.info.count; idx++) {
        vec_append(&a_main, &new_vec, uast_case_clone(vec_at(&vec, idx), use_new_scope, new_scope, dest_pos));
    }
    return new_vec;
}

Uast_symbol* uast_symbol_clone(const Uast_symbol* symbol, bool use_new_scope, Scope_id new_scope) {
    return uast_symbol_new(symbol->pos, name_clone(symbol->name, use_new_scope, new_scope));
}

Uast_member_access* uast_member_access_clone(const Uast_member_access* access, bool use_new_scope, Scope_id new_scope, Pos dest_pos) {
    return uast_member_access_new(
        access->pos,
        uast_symbol_clone(access->member_name, use_new_scope, new_scope),
        uast_expr_clone(access->callee, use_new_scope, new_scope, dest_pos)
    );
}

Uast_unary* uast_unary_clone(const Uast_unary* unary, bool use_new_scope, Scope_id new_scope, Pos dest_pos) {
    return uast_unary_new(
        unary->pos,
        uast_expr_clone(unary->child, use_new_scope, new_scope, dest_pos),
        unary->token_type,
        ulang_type_clone(unary->lang_type, use_new_scope, new_scope)
    );
}

Uast_binary* uast_binary_clone(const Uast_binary* binary, bool use_new_scope, Scope_id new_scope, Pos dest_pos) {
    return uast_binary_new(
        binary->pos,
        uast_expr_clone(binary->lhs, use_new_scope, new_scope, dest_pos),
        uast_expr_clone(binary->rhs, use_new_scope, new_scope, dest_pos),
        binary->token_type
    );
}

Uast_index* uast_index_clone(const Uast_index* index, bool use_new_scope, Scope_id new_scope, Pos dest_pos) {
    return uast_index_new(
        index->pos,
        uast_expr_clone(index->index, use_new_scope, new_scope, dest_pos),
        uast_expr_clone(index->callee, use_new_scope, new_scope, dest_pos)
    );
}

Uast_function_call* uast_function_call_clone(const Uast_function_call* fun_call, bool use_new_scope, Scope_id new_scope, Pos dest_pos) {
    return uast_function_call_new(
        fun_call->pos,
        uast_expr_vec_clone(fun_call->args, use_new_scope, new_scope, dest_pos),
        uast_expr_clone(fun_call->callee, use_new_scope, new_scope, dest_pos)
    );
}

Uast_struct_literal* uast_struct_literal_clone(const Uast_struct_literal* lit, bool use_new_scope, Scope_id new_scope, Pos dest_pos) {
    return uast_struct_literal_new(
        lit->pos,
        uast_expr_vec_clone(lit->members, use_new_scope, new_scope, dest_pos)
    );
}

Uast_tuple* uast_tuple_clone(const Uast_tuple* tuple, bool use_new_scope, Scope_id new_scope, Pos dest_pos) {
    return uast_tuple_new(tuple->pos, uast_expr_vec_clone(tuple->members, use_new_scope, new_scope, dest_pos));
}

Uast_macro* uast_macro_clone(const Uast_macro* macro, bool use_new_scope, Scope_id new_scope, Pos dest_pos) {
    (void) new_scope;
    (void) use_new_scope;
    return uast_macro_new(macro->pos, macro->name, dest_pos);
}

Uast_operator* uast_operator_clone(const Uast_operator* oper, bool use_new_scope, Scope_id new_scope, Pos dest_pos) {
    switch (oper->type) {
        case UAST_UNARY:
            return uast_unary_wrap(uast_unary_clone(uast_unary_const_unwrap(oper), use_new_scope, new_scope, dest_pos));
        case UAST_BINARY:
            return uast_binary_wrap(uast_binary_clone(uast_binary_const_unwrap(oper), use_new_scope, new_scope, dest_pos));
    }
    unreachable("");
}

Uast_unknown* uast_unknown_clone(const Uast_unknown* unknown) {
    return uast_unknown_new(unknown->pos);
}

Uast_param* uast_param_clone(const Uast_param* param, bool use_new_scope, Scope_id new_scope) {
    Uast_expr* new_opt_default = param->is_optional ? 
        uast_expr_clone(param->optional_default, use_new_scope, new_scope, (Pos) {0} /* TODO */) :
        NULL;

    return uast_param_new(
        param->pos,
        uast_variable_def_clone(param->base, use_new_scope, new_scope),
        param->is_optional,
        param->is_variadic,
        new_opt_default
    );
}

Uast_lang_def* uast_lang_def_clone(const Uast_lang_def* def) {
    (void) def;
    todo();
}

Uast_void_def* uast_void_def_clone(const Uast_void_def* def) {
    (void) def;
    return uast_void_def_new(POS_BUILTIN);
}

Uast_label* uast_label_clone(const Uast_label* label, bool use_new_scope, Scope_id new_scope) {
    assert(label->name.scope_id == label->block_scope);
    Scope_id scope = use_new_scope ? new_scope : label->name.scope_id;
    return uast_label_new(label->pos, name_clone(label->name, use_new_scope, new_scope), scope);
}

Uast_mod_alias* uast_mod_alias_clone(const Uast_mod_alias* alias, bool use_new_scope, Scope_id new_scope) {
    (void) alias;
    (void) new_scope;
    (void) use_new_scope;
    todo();
    //return uast_mod_alias_new(alias->pos, name_clone(alias->name, new_scope), name_clone(alias->mod_path, new_scope));
}

Uast_generic_param* uast_generic_param_clone(const Uast_generic_param* param, bool use_new_scope, Scope_id new_scope) {
    return uast_generic_param_new(param->pos, name_clone(param->name, use_new_scope, new_scope));
}

Uast_expr* uast_expr_clone(const Uast_expr* expr, bool use_new_scope, Scope_id new_scope, Pos dest_pos) {
    switch (expr->type) {
        case UAST_BLOCK:
            return uast_block_wrap(uast_block_clone(uast_block_const_unwrap(expr), use_new_scope, new_scope, dest_pos));
        case UAST_MACRO:
            return uast_macro_wrap(uast_macro_clone(uast_macro_const_unwrap(expr), use_new_scope, new_scope, dest_pos));
        case UAST_OPERATOR:
            return uast_operator_wrap(uast_operator_clone(uast_operator_const_unwrap(expr), use_new_scope, new_scope, dest_pos));
        case UAST_SYMBOL:
            return uast_symbol_wrap(uast_symbol_clone(uast_symbol_const_unwrap(expr), use_new_scope, new_scope));
        case UAST_MEMBER_ACCESS:
            return uast_member_access_wrap(uast_member_access_clone(uast_member_access_const_unwrap(expr), use_new_scope, new_scope, dest_pos));
        case UAST_INDEX:
            return uast_index_wrap(uast_index_clone(uast_index_const_unwrap(expr), use_new_scope, new_scope, dest_pos));
        case UAST_LITERAL:
            return uast_literal_wrap(uast_literal_clone(uast_literal_const_unwrap(expr)));
        case UAST_FUNCTION_CALL:
            return uast_function_call_wrap(uast_function_call_clone(uast_function_call_const_unwrap(expr), use_new_scope, new_scope, dest_pos));
        case UAST_STRUCT_LITERAL:
            return uast_struct_literal_wrap(uast_struct_literal_clone(uast_struct_literal_const_unwrap(expr), use_new_scope, new_scope, dest_pos));
        case UAST_TUPLE:
            return uast_tuple_wrap(uast_tuple_clone(uast_tuple_const_unwrap(expr), use_new_scope, new_scope, dest_pos));
        case UAST_ENUM_ACCESS: // TODO: remove uast_enum_access if not used
            todo();
        case UAST_UNKNOWN: // TODO: remove uast_enum_access if not used
            return uast_unknown_wrap(uast_unknown_clone(uast_unknown_const_unwrap(expr)));
        case UAST_ENUM_GET_TAG: // TODO: remove uast_enum_access if not used
            todo();
        case UAST_SWITCH:
            return uast_switch_wrap(uast_switch_clone(uast_switch_const_unwrap(expr), use_new_scope, new_scope, dest_pos));
        case UAST_IF_ELSE_CHAIN:
            return uast_if_else_chain_wrap(uast_if_else_chain_clone(uast_if_else_chain_const_unwrap(expr), use_new_scope, new_scope, dest_pos));
        case UAST_ARRAY_LITERAL:
            return uast_array_literal_wrap(uast_array_literal_clone(uast_array_literal_const_unwrap(expr), use_new_scope, new_scope, dest_pos));
    }
    unreachable("");
}

Uast_def* uast_def_clone(const Uast_def* def, bool use_new_scope, Scope_id new_scope) {
    switch (def->type) {
        case UAST_GENERIC_PARAM:
            return uast_generic_param_wrap(uast_generic_param_clone(uast_generic_param_const_unwrap(def), use_new_scope, new_scope));
        case UAST_FUNCTION_DEF:
            todo();
        case UAST_FUNCTION_DECL:
            todo();
        case UAST_STRUCT_DEF:
            todo();
        case UAST_RAW_UNION_DEF:
            todo();
        case UAST_ENUM_DEF:
            todo();
        case UAST_PRIMITIVE_DEF:
            todo();
        case UAST_VARIABLE_DEF: {
            // TODO: simplify
            Uast_variable_def* new_def = uast_variable_def_clone(uast_variable_def_const_unwrap(def), use_new_scope, new_scope);
            assert(uast_variable_def_const_unwrap(def) != new_def);
            assert(def != uast_variable_def_wrap(new_def));
            return uast_variable_def_wrap(new_def);
        }
        case UAST_POISON_DEF:
            todo();
        case UAST_MOD_ALIAS:
            return uast_mod_alias_wrap(uast_mod_alias_clone(uast_mod_alias_const_unwrap(def), use_new_scope, new_scope));
        case UAST_IMPORT_PATH:
            todo();
        case UAST_LANG_DEF:
            return uast_lang_def_wrap(uast_lang_def_clone(uast_lang_def_const_unwrap(def)));
        case UAST_VOID_DEF:
            return uast_void_def_wrap(uast_void_def_clone(uast_void_def_const_unwrap(def)));
        case UAST_LABEL:
            return uast_label_wrap(uast_label_clone(uast_label_const_unwrap(def), use_new_scope, new_scope));
    }
    unreachable("");
}

Uast_for_with_cond* uast_for_with_cond_clone(const Uast_for_with_cond* lang_for, bool use_new_scope, Scope_id new_scope, Pos dest_pos) {
    return uast_for_with_cond_new(
        lang_for->pos,
        uast_condition_clone(lang_for->condition, use_new_scope, new_scope, dest_pos),
        uast_block_clone(lang_for->body, use_new_scope, new_scope, dest_pos),
        name_clone(lang_for->continue_label, use_new_scope, new_scope),
        lang_for->do_cont_label
    );
}

Uast_condition* uast_condition_clone(const Uast_condition* cond, bool use_new_scope, Scope_id new_scope, Pos dest_pos) {
    return uast_condition_new(cond->pos, uast_operator_clone(cond->child, use_new_scope, new_scope, dest_pos));
}

Uast_yield* uast_yield_clone(const Uast_yield* yield, bool use_new_scope, Scope_id new_scope, Pos dest_pos) {
    return uast_yield_new(yield->pos, yield->do_yield_expr, uast_expr_clone(yield->yield_expr, use_new_scope, new_scope, dest_pos), name_clone(yield->break_out_of, use_new_scope, new_scope));
}

Uast_assignment* uast_assignment_clone(const Uast_assignment* assign, bool use_new_scope, Scope_id new_scope, Pos dest_pos) {
    return uast_assignment_new(
        assign->pos,
        uast_expr_clone(assign->lhs, use_new_scope, new_scope, dest_pos),
        uast_expr_clone(assign->rhs, use_new_scope, new_scope, dest_pos)
    );
}

Uast_return* uast_return_clone(const Uast_return* rtn, bool use_new_scope, Scope_id new_scope, Pos dest_pos) {
    return uast_return_new(rtn->pos, uast_expr_clone(rtn->child, use_new_scope, new_scope, dest_pos), rtn->is_auto_inserted);
}

Uast_if_else_chain* uast_if_else_chain_clone(const Uast_if_else_chain* if_else, bool use_new_scope, Scope_id new_scope, Pos dest_pos) {
    return uast_if_else_chain_new(if_else->pos, uast_if_vec_clone(if_else->uasts, use_new_scope, new_scope, dest_pos));
}

Uast_array_literal* uast_array_literal_clone(const Uast_array_literal* if_else, bool use_new_scope, Scope_id new_scope, Pos dest_pos) {
    return uast_array_literal_new(if_else->pos, uast_expr_vec_clone(if_else->members, use_new_scope, new_scope, dest_pos));
}

Uast_switch* uast_switch_clone(const Uast_switch* lang_switch, bool use_new_scope, Scope_id new_scope, Pos dest_pos) {
    return uast_switch_new(
        lang_switch->pos,
        uast_expr_clone(lang_switch->operand, use_new_scope, new_scope, dest_pos),
        uast_case_vec_clone(lang_switch->cases, use_new_scope, new_scope, dest_pos)
    );
}

Uast_defer* uast_defer_clone(const Uast_defer* lang_defer, bool use_new_scope, Scope_id new_scope, Pos dest_pos) {
    return uast_defer_new(lang_defer->pos, uast_stmt_clone(lang_defer->child, use_new_scope, new_scope, dest_pos));
}

Uast_stmt* uast_stmt_clone(const Uast_stmt* stmt, bool use_new_scope, Scope_id new_scope, Pos dest_pos) {
    switch (stmt->type) {
        case UAST_EXPR:
            return uast_expr_wrap(uast_expr_clone(uast_expr_const_unwrap(stmt), use_new_scope, new_scope, dest_pos));
        case UAST_DEF:
            return uast_def_wrap(uast_def_clone(uast_def_const_unwrap(stmt), use_new_scope, new_scope));
        case UAST_FOR_WITH_COND:
            return uast_for_with_cond_wrap(uast_for_with_cond_clone(uast_for_with_cond_const_unwrap(stmt), use_new_scope, new_scope, dest_pos));
        case UAST_YIELD:
            return uast_yield_wrap(uast_yield_clone(uast_yield_const_unwrap(stmt), use_new_scope, new_scope, dest_pos));
        case UAST_CONTINUE:
            // TODO
            todo();
        case UAST_ASSIGNMENT:
            return uast_assignment_wrap(uast_assignment_clone(uast_assignment_const_unwrap(stmt), use_new_scope, new_scope, dest_pos));
        case UAST_RETURN:
            return uast_return_wrap(uast_return_clone(uast_return_const_unwrap(stmt), use_new_scope, new_scope, dest_pos));
        case UAST_DEFER:
            return uast_defer_wrap(uast_defer_clone(uast_defer_const_unwrap(stmt), use_new_scope, new_scope, dest_pos));
    }
    unreachable("");
}

Uast_case* uast_case_clone(const Uast_case* lang_case, bool use_new_scope, Scope_id new_scope, Pos dest_pos) {
    return uast_case_new(
        lang_case->pos,
        lang_case->is_default,
        uast_expr_clone(lang_case->expr, use_new_scope, new_scope, dest_pos),
        uast_stmt_clone(lang_case->if_true, use_new_scope, new_scope, dest_pos),
        new_scope
    );
}

Uast_variable_def* uast_variable_def_clone(const Uast_variable_def* def, bool use_new_scope, Scope_id new_scope) {
    return uast_variable_def_new(def->pos, ulang_type_clone(def->lang_type, use_new_scope, new_scope), name_clone(def->name, use_new_scope, new_scope));
}

Uast_function_decl* uast_function_decl_clone(const Uast_function_decl* decl, bool use_new_scope, Scope_id new_scope) {
    return uast_function_decl_new(
        decl->pos,
        uast_generic_param_vec_clone(decl->generics, use_new_scope, new_scope),
        uast_function_params_clone(decl->params, use_new_scope, new_scope),
        ulang_type_clone(decl->return_type, use_new_scope, new_scope),
        name_clone(decl->name, use_new_scope, new_scope)
    );
}

Uast_function_params* uast_function_params_clone(const Uast_function_params* params, bool use_new_scope, Scope_id new_scope) {
    return uast_function_params_new(
        params->pos,
        uast_param_vec_clone(params->params, use_new_scope, new_scope)
    );
}

Uast_block* uast_block_clone(const Uast_block* block, bool use_new_scope, Scope_id parent, Pos dest_pos) {
    Uast_stmt_vec new_children = {0};
    Scope_id scope = use_new_scope ? scope_id_clone(block->scope_id, parent) : block->scope_id;
    for (size_t idx = 0; idx < block->children.info.count; idx++) {
        vec_append(&a_main, &new_children, uast_stmt_clone(vec_at(&block->children, idx), use_new_scope, scope, dest_pos));
    }
    return uast_block_new(block->pos, new_children, block->pos_end, scope);
}

Uast_if* uast_if_clone(const Uast_if* lang_if, bool use_new_scope, Scope_id new_scope, Pos dest_pos) {
    return uast_if_new(
        lang_if->pos,
        uast_condition_clone(lang_if->condition, use_new_scope, new_scope, dest_pos),
        uast_block_clone(lang_if->body, use_new_scope, new_scope, dest_pos)
    );
}
