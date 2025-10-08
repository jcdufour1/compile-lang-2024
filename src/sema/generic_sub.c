#include <generic_sub.h>
#include <ulang_type.h>
#include <ulang_type_clone.h>
#include <resolve_generics.h>
#include <uast_clone.h>
#include <uast_utils.h>
#include <env.h>
#include <symbol_iter.h>

void generic_sub_return(Uast_return* rtn, Name gen_param, Ulang_type gen_arg) {
    generic_sub_expr(rtn->child, gen_param, gen_arg);
}

void generic_sub_continue(Uast_continue* cont, Name gen_param, Ulang_type gen_arg) {
    (void) cont;
    (void) gen_param;
    (void) gen_arg;
}

void generic_sub_defer(Uast_defer* defer, Name gen_param, Ulang_type gen_arg) {
    generic_sub_stmt(defer->child, gen_param, gen_arg);
}

void generic_sub_using(Uast_using* using, Name gen_param, Ulang_type gen_arg) {
    generic_sub_name(&using->sym_name, gen_param, gen_arg);
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
    // TODO: call generic_sub_name here?
    Name temp = {0};

    unwrap(name_from_uname(&temp, lang_type.atom.str, lang_type.pos));
    if (name_is_equal(gen_param, temp)) {
        *new_lang_type = ulang_type_clone(gen_arg, true, lang_type.atom.str.scope_id);

        int16_t base_depth = lang_type.atom.pointer_depth;
        int16_t gen_prev_depth = ulang_type_get_pointer_depth(*new_lang_type);
        ulang_type_set_pointer_depth(new_lang_type, gen_prev_depth + base_depth);
        return;
    }

    lang_type = ulang_type_regular_clone(lang_type, true, lang_type.atom.str.scope_id);
    Ulang_type_vec* gen_args = &lang_type.atom.str.gen_args;
    for (size_t idx = 0; idx < gen_args->info.count; idx++) {
        generic_sub_lang_type(vec_at_ref(gen_args, idx), vec_at(gen_args, idx), gen_param, gen_arg);
    }
    *new_lang_type = ulang_type_regular_const_wrap(lang_type);
}

void generic_sub_lang_type_gen_param(
    Ulang_type* new_lang_type,
    Ulang_type_gen_param lang_type,
    Name gen_param,
    Ulang_type gen_arg
) {
    (void) gen_param;
    (void) gen_arg;
    *new_lang_type = ulang_type_gen_param_const_wrap(lang_type);
}

void generic_sub_lang_type_tuple(Ulang_type_tuple* lang_type, Name gen_param, Ulang_type gen_arg) {
    for (size_t idx = 0; idx < lang_type->ulang_types.info.count; idx++) {
        generic_sub_lang_type(vec_at_ref(&lang_type->ulang_types, idx), vec_at(&lang_type->ulang_types, idx), gen_param, gen_arg);
    }
}

void generic_sub_lang_type_fn(Ulang_type_fn* lang_type, Name gen_param, Ulang_type gen_arg) {
    generic_sub_lang_type_tuple(&lang_type->params, gen_param, gen_arg);
    generic_sub_lang_type(lang_type->return_type, *lang_type->return_type, gen_param, gen_arg);
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
        case ULANG_TYPE_FN: {
            Ulang_type_fn fn = ulang_type_fn_const_unwrap(lang_type);
            generic_sub_lang_type_fn(
                &fn,
                gen_param,
                gen_arg
            );
            *new_lang_type = ulang_type_fn_const_wrap(fn);
            return;
        }
        case ULANG_TYPE_TUPLE:
            todo();
        case ULANG_TYPE_GEN_PARAM:
            generic_sub_lang_type_gen_param(
                new_lang_type,
                ulang_type_gen_param_const_unwrap(lang_type),
                gen_param,
                gen_arg
            );
            return;
    }
    unreachable("");
}

void generic_sub_variable_def(Uast_variable_def* def, Name gen_param, Ulang_type gen_arg) {
    generic_sub_name(&def->name, gen_param, gen_arg);
    generic_sub_lang_type(&def->lang_type, def->lang_type, gen_param, gen_arg);
}

void generic_sub_generic_param(Uast_generic_param* def, Name gen_param, Ulang_type gen_arg) {
    generic_sub_name(&def->name, gen_param, gen_arg);
}

void generic_sub_label(Uast_label* label, Name gen_param, Ulang_type gen_arg) {
    generic_sub_name(&label->name, gen_param, gen_arg);
}

void generic_sub_struct_def_base(Ustruct_def_base* base, Name gen_param, Ulang_type gen_arg) {
    generic_sub_name(&base->name, gen_param, gen_arg);
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
            generic_sub_generic_param(uast_generic_param_unwrap(def), gen_param, gen_arg);
            return;
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
        case UAST_PRIMITIVE_DEF:
            todo();
        case UAST_FUNCTION_DECL:
            todo();
        case UAST_LANG_DEF:
            return;
        case UAST_VOID_DEF:
            todo();
        case UAST_LABEL:
            generic_sub_label(uast_label_unwrap(def), gen_param, gen_arg);
            return;
    }
    unreachable("");
}

void generic_sub_yield(Uast_yield* yield, Name gen_param, Ulang_type gen_arg) {
    if (yield->do_yield_expr) {
        generic_sub_expr(yield->yield_expr, gen_param, gen_arg);
    }
}

void generic_sub_stmt(Uast_stmt* stmt, Name gen_param, Ulang_type gen_arg) {
    switch (stmt->type) {
        case UAST_EXPR:
            generic_sub_expr(uast_expr_unwrap(stmt), gen_param, gen_arg);
            return;
        case UAST_DEF:
            generic_sub_def(uast_def_unwrap(stmt), gen_param, gen_arg);
            return;
        case UAST_FOR_WITH_COND:
            generic_sub_for_with_cond(uast_for_with_cond_unwrap(stmt), gen_param, gen_arg);
            return;
        case UAST_CONTINUE:
            generic_sub_continue(uast_continue_unwrap(stmt), gen_param, gen_arg);
            return;
        case UAST_ASSIGNMENT:
            generic_sub_assignment(uast_assignment_unwrap(stmt), gen_param, gen_arg);
            return;
        case UAST_RETURN:
            generic_sub_return(uast_return_unwrap(stmt), gen_param, gen_arg);
            return;
        case UAST_DEFER:
            generic_sub_defer(uast_defer_unwrap(stmt), gen_param, gen_arg);
            return;
        case UAST_USING:
            generic_sub_using(uast_using_unwrap(stmt), gen_param, gen_arg);
            return;
        case UAST_YIELD:
            generic_sub_yield(uast_yield_unwrap(stmt), gen_param, gen_arg);
            return;
        case UAST_STMT_REMOVED:
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

void generic_sub_array_literal(Uast_array_literal* lit, Name gen_param, Ulang_type gen_arg) {
    for (size_t idx = 0; idx < lit->members.info.count; idx++) {
        generic_sub_expr(vec_at(&lit->members, idx), gen_param, gen_arg);
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
    generic_sub_name(&lang_for->continue_label, gen_param, gen_arg);
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

void generic_sub_block(Uast_block* block, Name gen_param /* TODO: avoid using name for gen_param, because it has junk scope_id member (consider if Strv can be used for gen_param)*/, Ulang_type gen_arg) {
    assert(gen_param.scope_id > 0);
    Usymbol_iter iter = usym_tbl_iter_new(block->scope_id);
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
        case UAST_BLOCK:
            generic_sub_block(uast_block_unwrap(expr), gen_param, gen_arg);
            return;
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
            generic_sub_struct_literal(uast_struct_literal_unwrap(expr), gen_param, gen_arg);
            return;
        case UAST_TUPLE:
            msg_todo("", uast_expr_get_pos(expr));
            return;
        case UAST_ENUM_ACCESS:
            msg_todo("", uast_expr_get_pos(expr));
            return;
        case UAST_ENUM_GET_TAG:
            msg_todo("", uast_expr_get_pos(expr));
            return;
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
            generic_sub_array_literal(uast_array_literal_unwrap(expr), gen_param, gen_arg);
            return;
        case UAST_MACRO:
            // TODO
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

void generic_sub_struct_literal(Uast_struct_literal* lit, Name gen_param, Ulang_type gen_arg) {
    for (size_t idx = 0; idx < lit->members.info.count; idx++) {
        generic_sub_expr(vec_at(&lit->members, idx), gen_param, gen_arg);
    }
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
    generic_sub_name(&sym->name, gen_param, gen_arg);
}

void generic_sub_literal(Uast_literal* lit, Name gen_param, Ulang_type gen_arg) {
    (void) lit;
    (void) gen_param;
    (void) gen_arg;
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
}

void generic_sub_unary(Uast_unary* unary, Name gen_param, Ulang_type gen_arg) {
    generic_sub_expr(unary->child, gen_param, gen_arg);
    generic_sub_lang_type(&unary->lang_type, unary->lang_type, gen_param, gen_arg);
}

void generic_sub_name(Name* name, Name gen_param, Ulang_type gen_arg) {
    if (name_is_equal(*name, gen_param)) {
        if (name->gen_args.info.count > 0) {
            // TODO
            todo();
        }
        if (gen_arg.type == ULANG_TYPE_REGULAR) {
            if (!name_from_uname(name, ulang_type_regular_const_unwrap(gen_arg).atom.str, ulang_type_regular_const_unwrap(gen_arg).pos)) {
                // TODO
                todo();
            }
        } else if (gen_arg.type == ULANG_TYPE_GEN_PARAM) {
            unreachable("generic sub name should not be called with generic_parameter lang_type (lang_type should be substituted first)");
        } else {
            log(LOG_DEBUG, FMT"\n", name_print(NAME_LOG, gen_param));
            log(LOG_DEBUG, FMT"\n", ulang_type_print(LANG_TYPE_MODE_LOG, gen_arg));
            // TODO
            todo();
        }
        return;
    }
    for (size_t idx = 0; idx < name->gen_args.info.count; idx++) {
        generic_sub_lang_type(vec_at_ref(&name->gen_args, idx), vec_at(&name->gen_args, idx), gen_param, gen_arg);
    }
    // TODO
}
