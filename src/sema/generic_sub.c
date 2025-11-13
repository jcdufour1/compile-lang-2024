#include <generic_sub.h>
#include <ulang_type.h>
#include <ulang_type_clone.h>
#include <resolve_generics.h>
#include <uast_clone.h>
#include <uast_utils.h>
#include <env.h>
#include <symbol_iter.h>

void generic_sub_return(Uast_return* rtn, Name gen_param, Ulang_type gen_arg) {
    generic_sub_expr(&rtn->child, rtn->child, gen_param, gen_arg);
}

void generic_sub_continue(Uast_continue* cont, Name gen_param, Ulang_type gen_arg) {
    // TODO: should cont->break_out_of be substituted?
    (void) cont;
    (void) gen_param;
    (void) gen_arg;
}

void generic_sub_defer(Uast_defer* defer, Name gen_param, Ulang_type gen_arg) {
    generic_sub_stmt(&defer->child, defer->child, gen_param, gen_arg);
}

void generic_sub_using(Uast_using* using, Name gen_param, Ulang_type gen_arg) {
    Uast_int* dummy = NULL;
    int16_t pointer_depth = 0;
    switch (generic_sub_name(&dummy, &pointer_depth, &using->sym_name, using->pos, gen_param, gen_arg)) {
        case GEN_SUB_NAME_NORMAL:
            if (pointer_depth > 0) {
                msg_todo("", using->pos);
            }
            return;
        case GEN_SUB_NAME_NEW_INT:
            msg_todo("", using->pos);
            return;
        case GEN_SUB_NAME_ERROR:
            return;
    }
    unreachable("");
}

void generic_sub_param(Uast_param* def, Name gen_param, Ulang_type gen_arg) {
    generic_sub_variable_def(def->base, gen_param, gen_arg);
    if (def->is_optional) {
        generic_sub_expr(&def->optional_default, def->optional_default, gen_param, gen_arg);
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
        generic_sub_lang_type(vec_at_ref(gen_args, idx), vec_at(*gen_args, idx), gen_param, gen_arg);
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

void generic_sub_lang_type_expr(
    Ulang_type* new_lang_type,
    Ulang_type_expr lang_type,
    Name gen_param,
    Ulang_type gen_arg
) {
    generic_sub_expr(&lang_type.expr, lang_type.expr, gen_param, gen_arg);
    *new_lang_type = ulang_type_expr_const_wrap(lang_type);
}

void generic_sub_lang_type_tuple(Ulang_type_tuple* lang_type, Name gen_param, Ulang_type gen_arg) {
    for (size_t idx = 0; idx < lang_type->ulang_types.info.count; idx++) {
        generic_sub_lang_type(vec_at_ref(&lang_type->ulang_types, idx), vec_at(lang_type->ulang_types, idx), gen_param, gen_arg);
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
        case ULANG_TYPE_ARRAY:
            msg_todo("", ulang_type_get_pos(lang_type));
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
            msg_todo("", ulang_type_get_pos(lang_type));
            return;
        case ULANG_TYPE_GEN_PARAM:
            generic_sub_lang_type_gen_param(
                new_lang_type,
                ulang_type_gen_param_const_unwrap(lang_type),
                gen_param,
                gen_arg
            );
            return;
        case ULANG_TYPE_EXPR:
            generic_sub_lang_type_expr(
                new_lang_type,
                ulang_type_expr_const_unwrap(lang_type),
                gen_param,
                gen_arg
            );
            return;
        case ULANG_TYPE_INT:
            msg_todo("", ulang_type_get_pos(lang_type));
            return;
        case ULANG_TYPE_REMOVED:
            todo();
            return;
    }
    unreachable("");
}

void generic_sub_variable_def(Uast_variable_def* def, Name gen_param, Ulang_type gen_arg) {
    generic_sub_lang_type(&def->lang_type, def->lang_type, gen_param, gen_arg);
    Uast_int* dummy = NULL;
    int16_t ptr_depth_offset = 0;
    switch (generic_sub_name(&dummy, &ptr_depth_offset, &def->name, def->pos, gen_param, gen_arg)) {
        case GEN_SUB_NAME_NORMAL:
            ulang_type_add_pointer_depth(&def->lang_type, ptr_depth_offset);
            return;
        case GEN_SUB_NAME_NEW_INT:
            return;
        case GEN_SUB_NAME_ERROR:
            return;
    }
    unreachable("");
}

void generic_sub_label(Uast_label* label, Name gen_param, Ulang_type gen_arg) {
    Uast_int* dummy = NULL;
    int16_t ptr_depth_offset = 0;
    switch (generic_sub_name(&dummy, &ptr_depth_offset, &label->name, label->pos, gen_param, gen_arg)) {
        case GEN_SUB_NAME_NORMAL:
            return;
        case GEN_SUB_NAME_NEW_INT:
            msg_todo("", label->pos);
            return;
        case GEN_SUB_NAME_ERROR:
            return;
    }
    unreachable("");
}

void generic_sub_struct_def_base(Ustruct_def_base* base, Name gen_param, Ulang_type gen_arg) {
    for (size_t idx = 0; idx < base->members.info.count; idx++) {
        generic_sub_lang_type(&vec_at(base->members, idx)->lang_type, vec_at(base->members, idx)->lang_type, gen_param, gen_arg);
    }
    Pos pos = POS_BUILTIN;
    if (base->members.info.count > 0) {
        pos = vec_at(base->members, 0)->pos;
    }
    Uast_int* dummy = NULL;
    int16_t ptr_depth_offset = 0;
    switch (generic_sub_name(&dummy, &ptr_depth_offset, &base->name, pos, gen_param, gen_arg)) {
        case GEN_SUB_NAME_NORMAL:
            if (ptr_depth_offset > 0) {
                msg_todo("", vec_at(base->members, 0)->pos);
            }
            return;
        case GEN_SUB_NAME_NEW_INT:
            msg_todo("", pos);
            return;
        case GEN_SUB_NAME_ERROR:
            return;
    }
    unreachable("");
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
        case UAST_BUILTIN_DEF:
            todo();
    }
    unreachable("");
}

void generic_sub_yield(Uast_yield* yield, Name gen_param, Ulang_type gen_arg) {
    if (yield->do_yield_expr) {
        generic_sub_expr(&yield->yield_expr, yield->yield_expr, gen_param, gen_arg);
    }
}

void generic_sub_stmt(Uast_stmt** new_stmt, Uast_stmt* stmt, Name gen_param, Ulang_type gen_arg) {
    switch (stmt->type) {
        case UAST_EXPR: {
            Uast_expr* new_expr = NULL;
            generic_sub_expr(&new_expr, uast_expr_unwrap(stmt), gen_param, gen_arg);
            *new_stmt = uast_expr_wrap(new_expr);
            return;
        }
        case UAST_DEF:
            generic_sub_def(uast_def_unwrap(stmt), gen_param, gen_arg);
            *new_stmt = stmt;
            return;
        case UAST_FOR_WITH_COND:
            generic_sub_for_with_cond(uast_for_with_cond_unwrap(stmt), gen_param, gen_arg);
            *new_stmt = stmt;
            return;
        case UAST_CONTINUE:
            generic_sub_continue(uast_continue_unwrap(stmt), gen_param, gen_arg);
            *new_stmt = stmt;
            return;
        case UAST_ASSIGNMENT:
            generic_sub_assignment(uast_assignment_unwrap(stmt), gen_param, gen_arg);
            *new_stmt = stmt;
            return;
        case UAST_RETURN:
            generic_sub_return(uast_return_unwrap(stmt), gen_param, gen_arg);
            *new_stmt = stmt;
            return;
        case UAST_DEFER:
            generic_sub_defer(uast_defer_unwrap(stmt), gen_param, gen_arg);
            *new_stmt = stmt;
            return;
        case UAST_USING:
            generic_sub_using(uast_using_unwrap(stmt), gen_param, gen_arg);
            *new_stmt = stmt;
            return;
        case UAST_YIELD:
            generic_sub_yield(uast_yield_unwrap(stmt), gen_param, gen_arg);
            *new_stmt = stmt;
            return;
        case UAST_STMT_REMOVED:
            *new_stmt = stmt;
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
        generic_sub_if(vec_at(if_else->uasts, idx), gen_param, gen_arg);
    }
}

void generic_sub_array_literal(Uast_array_literal* lit, Name gen_param, Ulang_type gen_arg) {
    for (size_t idx = 0; idx < lit->members.info.count; idx++) {
        generic_sub_expr(vec_at_ref(&lit->members, idx), vec_at(lit->members, idx), gen_param, gen_arg);
    }
}

void generic_sub_switch(Uast_switch* lang_switch, Name gen_param, Ulang_type gen_arg) {
    generic_sub_expr(&lang_switch->operand, lang_switch->operand, gen_param, gen_arg);
    for (size_t idx = 0; idx < lang_switch->cases.info.count; idx++) {
        generic_sub_case(vec_at(lang_switch->cases, idx), gen_param, gen_arg);
    }
}

void generic_sub_for_with_cond(Uast_for_with_cond* lang_for, Name gen_param, Ulang_type gen_arg) {
    generic_sub_condition(lang_for->condition, gen_param, gen_arg);
    generic_sub_block(lang_for->body, gen_param, gen_arg);
    Uast_int* dummy = NULL;
    int16_t ptr_depth_offset = 0;
    switch (generic_sub_name(&dummy, &ptr_depth_offset, &lang_for->continue_label, lang_for->pos, gen_param, gen_arg)) {
        case GEN_SUB_NAME_NORMAL:
            if (ptr_depth_offset > 0) {
                msg_todo("", lang_for->pos);
            }
            return;
        case GEN_SUB_NAME_NEW_INT:
            msg_todo("", lang_for->pos);
            return;
        case GEN_SUB_NAME_ERROR:
            return;
    }
}

void generic_sub_condition(Uast_condition* cond, Name gen_param, Ulang_type gen_arg) {
    generic_sub_operator(cond->child, gen_param, gen_arg);
}

void generic_sub_case(Uast_case* lang_case, Name gen_param, Ulang_type gen_arg) {
    if (!lang_case->is_default) {
        generic_sub_expr(&lang_case->expr, lang_case->expr, gen_param, gen_arg);
    }
    generic_sub_block(lang_case->if_true, gen_param, gen_arg);
}

void generic_sub_assignment(Uast_assignment* assign, Name gen_param, Ulang_type gen_arg) {
    generic_sub_expr(&assign->lhs, assign->lhs, gen_param, gen_arg);
    generic_sub_expr(&assign->rhs, assign->rhs, gen_param, gen_arg);
}

void generic_sub_block(Uast_block* block, Name gen_param, Ulang_type gen_arg) {
    Usymbol_iter iter = usym_tbl_iter_new(block->scope_id);
    Uast_def* curr = NULL;
    while (usym_tbl_iter_next(&curr, &iter)) {
        assert(gen_arg.type != ULANG_TYPE_EXPR);
        generic_sub_def(curr, gen_param, gen_arg);
    }

    for (size_t idx = 0; idx < block->children.info.count; idx++) {
        generic_sub_stmt(vec_at_ref(&block->children, idx), vec_at(block->children, idx), gen_param, gen_arg);
    }
}

void generic_sub_expr(Uast_expr** new_expr, Uast_expr* expr, Name gen_param, Ulang_type gen_arg) {
    switch (expr->type) {
        case UAST_BLOCK:
            *new_expr = expr;
            generic_sub_block(uast_block_unwrap(expr), gen_param, gen_arg);
            return;
        case UAST_LITERAL:
            *new_expr = expr;
            generic_sub_literal(uast_literal_unwrap(expr), gen_param, gen_arg);
            return;
        case UAST_UNKNOWN:
            *new_expr = expr;
            return;
        case UAST_SYMBOL:
            generic_sub_symbol(new_expr, uast_symbol_unwrap(expr), gen_param, gen_arg);
            return;
        case UAST_MEMBER_ACCESS:
            generic_sub_member_access(new_expr, uast_member_access_unwrap(expr), gen_param, gen_arg);
            return;
        case UAST_INDEX:
            *new_expr = expr;
            generic_sub_index(uast_index_unwrap(expr), gen_param, gen_arg);
            return;
        case UAST_FUNCTION_CALL:
            *new_expr = expr;
            generic_sub_function_call(uast_function_call_unwrap(expr), gen_param, gen_arg);
            return;
        case UAST_STRUCT_LITERAL:
            *new_expr = expr;
            generic_sub_struct_literal(uast_struct_literal_unwrap(expr), gen_param, gen_arg);
            return;
        case UAST_TUPLE:
            *new_expr = expr;
            msg_todo("", uast_expr_get_pos(expr));
            return;
        case UAST_ENUM_ACCESS:
            *new_expr = expr;
            msg_todo("", uast_expr_get_pos(expr));
            return;
        case UAST_ENUM_GET_TAG:
            *new_expr = expr;
            msg_todo("", uast_expr_get_pos(expr));
            return;
        case UAST_OPERATOR:
            *new_expr = expr;
            generic_sub_operator(uast_operator_unwrap(expr), gen_param, gen_arg);
            return;
        case UAST_SWITCH:
            *new_expr = expr;
            generic_sub_switch(uast_switch_unwrap(expr), gen_param, gen_arg);
            return;
        case UAST_IF_ELSE_CHAIN:
            *new_expr = expr;
            generic_sub_if_else_chain(uast_if_else_chain_unwrap(expr), gen_param, gen_arg);
            return;
        case UAST_ARRAY_LITERAL:
            *new_expr = expr;
            generic_sub_array_literal(uast_array_literal_unwrap(expr), gen_param, gen_arg);
            return;
        case UAST_MACRO:
            *new_expr = expr;
            // TODO
            return;
        case UAST_EXPR_REMOVED:
            *new_expr = expr;
            return;
    }
    unreachable("");
}

void generic_sub_function_call(Uast_function_call* fun_call, Name gen_param, Ulang_type gen_arg) {
    for (size_t idx = 0; idx < fun_call->args.info.count; idx++) {
        generic_sub_expr(vec_at_ref(&fun_call->args, idx), vec_at(fun_call->args, idx), gen_param, gen_arg);
    }
    generic_sub_expr(&fun_call->callee, fun_call->callee, gen_param, gen_arg);
}

void generic_sub_struct_literal(Uast_struct_literal* lit, Name gen_param, Ulang_type gen_arg) {
    for (size_t idx = 0; idx < lit->members.info.count; idx++) {
        generic_sub_expr(vec_at_ref(&lit->members, idx), vec_at(lit->members, idx), gen_param, gen_arg);
    }
}

void generic_sub_member_access(Uast_expr** new_expr, Uast_member_access* access, Name gen_param, Ulang_type gen_arg) {
    generic_sub_expr(&access->callee, access->callee, gen_param, gen_arg);
    switch (generic_sub_symbol(new_expr, access->member_name, gen_param, gen_arg)) {
        case GEN_SUB_NAME_NORMAL:
            *new_expr = uast_member_access_wrap(access);
            return;
        case GEN_SUB_NAME_NEW_INT:
            return;
        case GEN_SUB_NAME_ERROR:
            return;
    }
    unreachable("");
}

void generic_sub_index(Uast_index* index, Name gen_param, Ulang_type gen_arg) {
    generic_sub_expr(&index->index, index->index, gen_param, gen_arg);
    generic_sub_expr(&index->callee, index->callee, gen_param, gen_arg);
}

GEN_SUB_NAME_STATUS generic_sub_symbol(Uast_expr** new_sym, Uast_symbol* sym, Name gen_param, Ulang_type gen_arg) {
    Uast_int* new_int = NULL;
    int16_t ptr_depth_offset = 0;
    switch (generic_sub_name(&new_int, &ptr_depth_offset, &sym->name, sym->pos, gen_param, gen_arg)) {
        case GEN_SUB_NAME_NORMAL:
            if (ptr_depth_offset <= 0) {
                return GEN_SUB_NAME_NORMAL;
            }

            Uast_binary* bin = uast_binary_new(
                sym->pos,
                uast_symbol_wrap(sym),
                uast_expr_removed_wrap(uast_expr_removed_new(
                    sym->pos,
                    sv("after binary operator")
                )),
                BINARY_MULTIPLY
            );
            for (int16_t idx = 1; idx < ptr_depth_offset; idx++) {
                bin->rhs = uast_operator_wrap(uast_unary_wrap(
                    uast_unary_new(sym->pos, bin->rhs, UNARY_DEREF, ulang_type_new_void(sym->pos))
                ));
            }
            *new_sym = uast_operator_wrap(uast_binary_wrap(bin));
            return GEN_SUB_NAME_NORMAL;
        case GEN_SUB_NAME_NEW_INT:
            *new_sym = uast_literal_wrap(uast_int_wrap(new_int));
            return GEN_SUB_NAME_NEW_INT;
        case GEN_SUB_NAME_ERROR:
            return GEN_SUB_NAME_ERROR;
    }
    unreachable("");
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
    generic_sub_expr(&bin->lhs, bin->lhs, gen_param, gen_arg);
    generic_sub_expr(&bin->rhs, bin->rhs, gen_param, gen_arg);
}

void generic_sub_unary(Uast_unary* unary, Name gen_param, Ulang_type gen_arg) {
    generic_sub_expr(&unary->child, unary->child, gen_param, gen_arg);
    generic_sub_lang_type(&unary->lang_type, unary->lang_type, gen_param, gen_arg);
}

GEN_SUB_NAME_STATUS generic_sub_name(
    Uast_int** new_expr,
    int16_t* pointer_depth_to_add, // this variable should be used if and only if 
                                   //   GEN_SUB_NAME_NORMAL is returned
    Name* name,
    Pos name_pos,
    Name gen_param,
    Ulang_type gen_arg
) {
    *pointer_depth_to_add = 0;

    if (name_is_equal(*name, gen_param)) {
        *pointer_depth_to_add = ulang_type_get_pointer_depth(gen_arg);
        
        if (name->gen_args.info.count > 0) {
            // TODO
            msg_todo("", name_pos);
            return GEN_SUB_NAME_ERROR;
        }

        switch (gen_arg.type) {
            case ULANG_TYPE_REGULAR: {
                Ulang_type_regular reg = ulang_type_regular_const_unwrap(gen_arg);
                if (name_from_uname(name, reg.atom.str, reg.pos)) {
                    return GEN_SUB_NAME_NORMAL;
                }
                return GEN_SUB_NAME_ERROR;
            }
            case ULANG_TYPE_GEN_PARAM:
                unreachable("generic sub name should not be called with generic_parameter lang_type (lang_type should be substituted first)");
            case ULANG_TYPE_INT:
                *new_expr = uast_int_new(name_pos, ulang_type_int_const_unwrap(gen_arg).data);
                return GEN_SUB_NAME_NEW_INT;
            case ULANG_TYPE_TUPLE:
                msg_todo("", name_pos);
                return GEN_SUB_NAME_ERROR;
            case ULANG_TYPE_FN:
                msg_todo("", name_pos);
                return GEN_SUB_NAME_ERROR;
            case ULANG_TYPE_ARRAY:
                msg_todo("", name_pos);
                return GEN_SUB_NAME_ERROR;
            case ULANG_TYPE_REMOVED:
                msg_todo("", name_pos);
                return GEN_SUB_NAME_ERROR;
            case ULANG_TYPE_EXPR:
                unreachable("gen_arg should not still be ulang_type_expr");
        }
        unreachable("");
    }

    for (size_t idx = 0; idx < name->gen_args.info.count; idx++) {
        generic_sub_lang_type(vec_at_ref(&name->gen_args, idx), vec_at(name->gen_args, idx), gen_param, gen_arg);
    }

    return GEN_SUB_NAME_NORMAL;
}
