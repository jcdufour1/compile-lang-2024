#include <generic_sub.h>
#include <ulang_type.h>
#include <ulang_type_clone.h>
#include <resolve_generics.h>
#include <uast_clone.h>
#include <uast_utils.h>
#include <env.h>
#include <symbol_iter.h>
#include <parser_utils.h>

void generic_sub_return(Uast_return* rtn, Name gen_param, Ulang_type gen_arg) {
    generic_sub_expr(rtn->child, gen_param, gen_arg);
}

void generic_sub_param(Uast_param* def, Name gen_param, Ulang_type gen_arg) {
    generic_sub_variable_def(def->base, gen_param, gen_arg);
    if (def->is_optional) {
        generic_sub_expr(def->optional_default, gen_param, gen_arg);
    }
}

void generic_sub_lang_type_regular(
    Ulang_type* new_lang_type,
    Ulang_type_regular lang_type,
    Name gen_param,
    Ulang_type gen_arg
) {
    Name temp = {0};

    unwrap(name_from_uname(&temp, lang_type.atom.str));
    if (name_is_equal(gen_param, temp)) {
        *new_lang_type = ulang_type_clone(gen_arg, lang_type.atom.str.scope_id);

        int16_t base_depth = lang_type.atom.pointer_depth;
        int16_t gen_prev_depth = ulang_type_get_pointer_depth(*new_lang_type);
        ulang_type_set_pointer_depth(new_lang_type, gen_prev_depth + base_depth);
        //log(LOG_DEBUG, TAST_FMT"\n", ulang_type_print(LANG_TYPE_MODE_MSG, *new_lang_type));
        return;
    }

    lang_type = ulang_type_regular_clone(lang_type, lang_type.atom.str.scope_id);
    Ulang_type_vec* gen_args = &lang_type.atom.str.gen_args;
    log(LOG_DEBUG, TAST_FMT"\n", ulang_type_print(LANG_TYPE_MODE_MSG, ulang_type_regular_const_wrap(lang_type)));
    for (size_t idx = 0; idx < gen_args->info.count; idx++) {
        generic_sub_lang_type(vec_at_ref(gen_args, idx), vec_at(gen_args, idx), gen_param, gen_arg);
    }
    log(LOG_DEBUG, TAST_FMT"\n", ulang_type_print(LANG_TYPE_MODE_MSG, ulang_type_regular_const_wrap(lang_type)));
    *new_lang_type = ulang_type_regular_const_wrap(lang_type);
}

void generic_sub_lang_type(
    Ulang_type* new_lang_type,
    Ulang_type lang_type,
    Name gen_param,
    Ulang_type gen_arg
) {
    (void) env;
    switch (lang_type.type) {
        case ULANG_TYPE_REGULAR:
            generic_sub_lang_type_regular(
                new_lang_type,
                ulang_type_regular_const_unwrap(lang_type),
                gen_param,
                gen_arg
            );
            return;
        case ULANG_TYPE_FN:
            todo();
        case ULANG_TYPE_TUPLE:
            todo();
    }
    unreachable("");
}

void generic_sub_variable_def(Uast_variable_def* def, Name gen_param, Ulang_type gen_arg) {
    generic_sub_lang_type(&def->lang_type, def->lang_type, gen_param, gen_arg);
}

void generic_sub_struct_def_base(Ustruct_def_base* base, Name gen_param, Ulang_type gen_arg) {
    for (size_t idx = 0; idx < base->members.info.count; idx++) {
        generic_sub_lang_type(&vec_at(&base->members, idx)->lang_type, vec_at(&base->members, idx)->lang_type, gen_param, gen_arg);
    }
}

void generic_sub_def(Uast_def* def, Name gen_param, Ulang_type gen_arg) {
    switch (def->type) {
        case UAST_MOD_ALIAS:
            todo();
        case UAST_IMPORT_PATH:
            todo();
        case UAST_POISON_DEF:
            todo();
        case UAST_GENERIC_PARAM:
            todo();
        case UAST_FUNCTION_DEF:
            todo();
        case UAST_VARIABLE_DEF:
            generic_sub_variable_def(uast_variable_def_unwrap(def), gen_param, gen_arg);
            return;
        case UAST_STRUCT_DEF:
            todo();
        case UAST_RAW_UNION_DEF:
            todo();
        case UAST_ENUM_DEF:
            todo();
        case UAST_SUM_DEF:
            todo();
        case UAST_PRIMITIVE_DEF:
            todo();
        case UAST_FUNCTION_DECL:
            todo();
        case UAST_LITERAL_DEF:
            todo();
        case UAST_LANG_DEF:
            todo();
    }
    unreachable("");
}

void generic_sub_continue(Uast_continue* cont) {
    (void) cont;
}

void generic_sub_label(Uast_label* label) {
    (void) label;
}

void generic_sub_stmt(Uast_stmt* stmt, Name gen_param, Ulang_type gen_arg) {
    switch (stmt->type) {
        case UAST_BLOCK:
            generic_sub_block(uast_block_unwrap(stmt), gen_param, gen_arg);
            return;
        case UAST_EXPR:
            generic_sub_expr(uast_expr_unwrap(stmt), gen_param, gen_arg);
            return;
        case UAST_DEF:
            generic_sub_def(uast_def_unwrap(stmt), gen_param, gen_arg);
            return;
        case UAST_FOR_WITH_COND:
            generic_sub_for_with_cond(uast_for_with_cond_unwrap(stmt), gen_param, gen_arg);
            return;
        case UAST_BREAK:
            todo();
        case UAST_CONTINUE:
            generic_sub_continue(uast_continue_unwrap(stmt));
            return;
        case UAST_ASSIGNMENT:
            generic_sub_assignment(uast_assignment_unwrap(stmt), gen_param, gen_arg);
            return;
        case UAST_RETURN:
            generic_sub_return(uast_return_unwrap(stmt), gen_param, gen_arg);
            return;
        case UAST_LABEL:
            generic_sub_label(uast_label_unwrap(stmt));
            return;
    }
    unreachable("");
}

void generic_sub_if(Uast_if* lang_if, Name gen_param, Ulang_type gen_arg) {
    generic_sub_condition(lang_if->condition, gen_param, gen_arg);
    generic_sub_block(lang_if->body, gen_param, gen_arg);
}

void generic_sub_if_else_chain(Uast_if_else_chain* if_else, Name gen_param, Ulang_type gen_arg) {
    for (size_t idx = 0; idx < if_else->uasts.info.count; idx++) {
        generic_sub_if(vec_at(&if_else->uasts, idx), gen_param, gen_arg);
    }
}

void generic_sub_switch(Uast_switch* lang_switch, Name gen_param, Ulang_type gen_arg) {
    generic_sub_expr(lang_switch->operand, gen_param, gen_arg);
    for (size_t idx = 0; idx < lang_switch->cases.info.count; idx++) {
        generic_sub_case(vec_at(&lang_switch->cases, idx), gen_param, gen_arg);
    }
}

void generic_sub_for_with_cond(Uast_for_with_cond* lang_for, Name gen_param, Ulang_type gen_arg) {
    generic_sub_condition(lang_for->condition, gen_param, gen_arg);
    generic_sub_block(lang_for->body, gen_param, gen_arg);
}

void generic_sub_condition(Uast_condition* cond, Name gen_param, Ulang_type gen_arg) {
    generic_sub_operator(cond->child, gen_param, gen_arg);
}

void generic_sub_case(Uast_case* lang_case, Name gen_param, Ulang_type gen_arg) {
    generic_sub_expr(lang_case->expr, gen_param, gen_arg);
    generic_sub_stmt(lang_case->if_true, gen_param, gen_arg);
}

void generic_sub_assignment(Uast_assignment* assign, Name gen_param, Ulang_type gen_arg) {
    generic_sub_expr(assign->lhs, gen_param, gen_arg);
    generic_sub_expr(assign->rhs, gen_param, gen_arg);
}

void generic_sub_block(Uast_block* block, Name gen_param, Ulang_type gen_arg) {
    Usymbol_iter iter = usym_tbl_iter_new(gen_param.scope_id);
    Uast_def* curr = NULL;
    while (usym_tbl_iter_next(&curr, &iter)) {
        generic_sub_def(curr, gen_param, gen_arg);
    }

    for (size_t idx = 0; idx < block->children.info.count; idx++) {
        generic_sub_stmt(vec_at(&block->children, idx), gen_param, gen_arg);
    }
}

void generic_sub_expr(Uast_expr* expr, Name gen_param, Ulang_type gen_arg) {
    switch (expr->type) {
        case UAST_LITERAL:
            generic_sub_literal(uast_literal_unwrap(expr), gen_param, gen_arg);
            return;
        case UAST_UNKNOWN:
            return;
        case UAST_SYMBOL:
            generic_sub_symbol(uast_symbol_unwrap(expr), gen_param, gen_arg);
            return;
        case UAST_MEMBER_ACCESS:
            generic_sub_member_access(uast_member_access_unwrap(expr), gen_param, gen_arg);
            return;
        case UAST_INDEX:
            generic_sub_index(uast_index_unwrap(expr), gen_param, gen_arg);
            return;
        case UAST_FUNCTION_CALL:
            generic_sub_function_call(uast_function_call_unwrap(expr), gen_param, gen_arg);
            return;
        case UAST_STRUCT_LITERAL:
            todo();
        case UAST_TUPLE:
            todo();
        case UAST_SUM_ACCESS:
            todo();
        case UAST_SUM_GET_TAG:
            todo();
        case UAST_OPERATOR:
            generic_sub_operator(uast_operator_unwrap(expr), gen_param, gen_arg);
            return;
        case UAST_SWITCH:
            generic_sub_switch(uast_switch_unwrap(expr), gen_param, gen_arg);
            return;
        case UAST_IF_ELSE_CHAIN:
            generic_sub_if_else_chain(uast_if_else_chain_unwrap(expr), gen_param, gen_arg);
            return;
        case UAST_ARRAY_LITERAL:
            todo();
            return;
    }
    unreachable("");
}

void generic_sub_function_call(Uast_function_call* fun_call, Name gen_param, Ulang_type gen_arg) {
    for (size_t idx = 0; idx < fun_call->args.info.count; idx++) {
        generic_sub_expr(vec_at(&fun_call->args, idx), gen_param, gen_arg);
    }
    generic_sub_expr(fun_call->callee, gen_param, gen_arg);
}

void generic_sub_member_access(Uast_member_access* access, Name gen_param, Ulang_type gen_arg) {
    generic_sub_expr(access->callee, gen_param, gen_arg);
    generic_sub_symbol(access->member_name, gen_param, gen_arg);
}

void generic_sub_index(Uast_index* index, Name gen_param, Ulang_type gen_arg) {
    generic_sub_expr(index->index, gen_param, gen_arg);
    generic_sub_expr(index->callee, gen_param, gen_arg);
}

void generic_sub_symbol(Uast_symbol* sym, Name gen_param, Ulang_type gen_arg) {
    // TODO: call gen_sub_name instead of this for loop
    log(LOG_DEBUG, TAST_FMT"\n", uast_symbol_print(sym));
    for (size_t idx = 0; idx < sym->name.gen_args.info.count; idx++) {
        log(LOG_DEBUG, TAST_FMT"\n", ulang_type_print(LANG_TYPE_MODE_MSG, vec_at(&sym->name.gen_args, idx)));
        generic_sub_lang_type(vec_at_ref(&sym->name.gen_args, idx), vec_at(&sym->name.gen_args, idx), gen_param, gen_arg);
    }
}

void generic_sub_literal(Uast_literal* lit, Name gen_param, Ulang_type gen_arg) {
    (void) lit;
    (void) gen_param;
    (void) gen_arg;
    return;
}

void generic_sub_operator(Uast_operator* oper, Name gen_param, Ulang_type gen_arg) {
    switch (oper->type) {
        case UAST_UNARY:
            generic_sub_unary(uast_unary_unwrap(oper), gen_param, gen_arg);
            return;
        case UAST_BINARY:
            generic_sub_binary(uast_binary_unwrap(oper), gen_param, gen_arg);
            return;
    }
    unreachable("");
}

void generic_sub_binary(Uast_binary* bin, Name gen_param, Ulang_type gen_arg) {
    generic_sub_expr(bin->lhs, gen_param, gen_arg);
    generic_sub_expr(bin->rhs, gen_param, gen_arg);
    return;
}

void generic_sub_unary(Uast_unary* unary, Name gen_param, Ulang_type gen_arg) {
    (void) env;
    (void) unary;
    (void) gen_param;
    (void) gen_arg;
}
