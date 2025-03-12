#include <uast_clone.h>
#include <parser_utils.h>
#include <symbol_collection_clone.h>

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
            todo();
    }
    unreachable("");
}

Ulang_type_vec ulang_type_vec_clone(Ulang_type_vec vec) {
    Ulang_type_vec new_vec = {0};
    for (size_t idx = 0; idx < vec.info.count; idx++) {
        vec_append(&a_main, &new_vec, vec_at(&vec, idx));
    }
    return new_vec;
}

Uast_symbol* uast_symbol_clone(const Uast_symbol* symbol) {
    return uast_symbol_new(symbol->pos, symbol->name, ulang_type_vec_clone(symbol->generic_args));
}

Uast_member_access* uast_member_access_clone(const Uast_member_access* access) {
    return uast_member_access_new(access->pos, access->member_name, uast_expr_clone(access->callee));
}

Uast_unary* uast_unary_clone(const Uast_unary* unary) {
    return uast_unary_new(unary->pos, uast_expr_clone(unary->child), unary->token_type, unary->lang_type);
}

Uast_binary* uast_binary_clone(const Uast_binary* binary) {
    return uast_binary_new(binary->pos, uast_stmt_clone(binary->lhs), uast_expr_clone(binary->rhs), binary->token_type);
}

Uast_index* uast_index_clone(const Uast_index* index) {
    return uast_index_new(index->pos, uast_expr_clone(index->index), uast_expr_clone(index->callee));
}

Uast_expr_vec uast_expr_vec_clone(Uast_expr_vec vec) {
    Uast_expr_vec new_vec = {0};
    for (size_t idx = 0; idx < vec.info.count; idx++) {
        vec_append(&a_main, &new_vec, uast_expr_clone(vec_at(&vec, idx)));
    }
    return new_vec;
}

Uast_function_call* uast_function_call_clone(const Uast_function_call* fun_call) {
    return uast_function_call_new(
        fun_call->pos,
        uast_expr_vec_clone(fun_call->args),
        uast_expr_clone(fun_call->callee)
    );
}

Uast_tuple* uast_tuple_clone(const Uast_tuple* tuple) {
    return uast_tuple_new(
        tuple->pos,
        uast_expr_vec_clone(tuple->members)
    );
}

Uast_operator* uast_operator_clone(const Uast_operator* oper) {
    switch (oper->type) {
        case UAST_UNARY:
            return uast_unary_wrap(uast_unary_clone(uast_unary_const_unwrap(oper)));
        case UAST_BINARY:
            return uast_binary_wrap(uast_binary_clone(uast_binary_const_unwrap(oper)));
    }
    unreachable("");
}

Uast_unknown* uast_unknown_clone(const Uast_unknown* unknown) {
    return uast_unknown_new(unknown->pos);
}

Uast_param* uast_param_clone(const Uast_param* param) {
    return uast_param_new(param->pos, uast_variable_def_clone(param->base), param->is_optional, param->is_variadic, param->optional_default);
}

Uast_expr* uast_expr_clone(const Uast_expr* expr) {
    switch (expr->type) {
        case UAST_OPERATOR:
            return uast_operator_wrap(uast_operator_clone(uast_operator_const_unwrap(expr)));
        case UAST_SYMBOL:
            return uast_symbol_wrap(uast_symbol_clone(uast_symbol_const_unwrap(expr)));
        case UAST_MEMBER_ACCESS:
            return uast_member_access_wrap(uast_member_access_clone(uast_member_access_const_unwrap(expr)));
        case UAST_INDEX:
            return uast_index_wrap(uast_index_clone(uast_index_const_unwrap(expr)));
        case UAST_LITERAL:
            return uast_literal_wrap(uast_literal_clone(uast_literal_const_unwrap(expr)));
        case UAST_FUNCTION_CALL:
            return uast_function_call_wrap(uast_function_call_clone(uast_function_call_const_unwrap(expr)));
        case UAST_STRUCT_LITERAL:
            todo();
        case UAST_TUPLE:
            return uast_tuple_wrap(uast_tuple_clone(uast_tuple_const_unwrap(expr)));
        case UAST_SUM_ACCESS: // TODO: remove uast_sum_access if not used
            todo();
        case UAST_UNKNOWN: // TODO: remove uast_sum_access if not used
            return uast_unknown_wrap(uast_unknown_clone(uast_unknown_const_unwrap(expr)));
    }
    unreachable("");
}

Uast_stmt* uast_stmt_clone(const Uast_stmt* stmt) {
    if (stmt->type != UAST_EXPR) {
        todo();
    }
    return uast_expr_wrap(uast_expr_clone(uast_expr_const_unwrap(stmt)));
}

Uast_variable_def* uast_variable_def_clone(const Uast_variable_def* def) {
    return uast_variable_def_new(def->pos, def->lang_type, def->name);
}

Uast_block* uast_block_clone(const Uast_block* block) {
    Uast_stmt_vec new_children = {0};
    for (size_t idx = 0; idx < block->children.info.count; idx++) {
        vec_append(&a_main, &new_children, vec_at(&block->children, idx));
    }
    return uast_block_new(block->pos, new_children, symbol_collection_clone(block->symbol_collection), block->pos_end);
}

