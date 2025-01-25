
#include "passes.h"
#include <util.h>
#include <llvm_utils.h>
#include <tast_utils.h>
#include <uast_utils.h>
#include <parser_utils.h>
#include <type_checking.h>
#include <serialize.h>
#include <lang_type_from_ulang_type.h>

static Tast_stmt* rm_tuple_stmt(Env* env, Tast_stmt* stmt);

static Tast_expr* rm_tuple_expr_rhs(Env* env, Lang_type lhs_lang_type, Tast_expr* rhs, Pos assign_pos);

static Tast_block* rm_tuple_block(Env* env, Tast_block* block);

static Tast_for_range* rm_tuple_for_range(Env* env, Tast_for_range* lang_for);

static Tast_for_with_cond* rm_tuple_for_with_cond(Env* env, Tast_for_with_cond* lang_for);

static Tast_stmt* rm_tuple_assignment(Env* env, Tast_assignment* assign);

static Tast_return* rm_tuple_return(Env* env, Tast_return* rtn);

static Tast_if* rm_tuple_if(Env* env, Tast_if* assign);

static Tast_if_else_chain* rm_tuple_if_else_chain(Env* env, Tast_if_else_chain* if_else);

static Tast_expr* rm_tuple_expr_not_in_assignment(Env* env, Tast_expr* expr);

static Tast_expr* rm_tuple_generic_assignment_rhs(
    Env* env,
    Tast_expr* rhs,
    Lang_type lhs_lang_type,
    Pos assign_pos
);

static Tast_for_range* rm_tuple_for_range(Env* env, Tast_for_range* lang_for) {
    // TODO: tuples in for lower bound, etc.
    lang_for->body = rm_tuple_block(env, lang_for->body);
    return lang_for;
}

static Tast_for_with_cond* rm_tuple_for_with_cond(Env* env, Tast_for_with_cond* lang_for) {
    // TODO: tuples in for condition, etc.
    lang_for->body = rm_tuple_block(env, lang_for->body);
    return lang_for;
}

static Tast_stmt* rm_tuple_assignment(Env* env, Tast_assignment* assign) {
    assign->rhs = rm_tuple_expr_rhs(env, tast_get_lang_type_stmt(assign->lhs), assign->rhs, assign->pos);

    if (tast_get_lang_type_stmt(assign->lhs).type != LANG_TYPE_TUPLE && tast_get_lang_type_expr(assign->rhs).type != LANG_TYPE_TUPLE) {
        return tast_wrap_assignment(assign);
    }

    Tast_stmt_vec new_children = {0};

    Tast_tuple* dest = tast_unwrap_tuple(tast_unwrap_expr(assign->lhs));
    Tast_function_call* src = tast_unwrap_function_call(assign->rhs);

    Uast_def* def_ = NULL;
    try(usymbol_lookup(&def_, env, src->name));
    Uast_function_decl* decl = uast_unwrap_function_decl(def_);
    log(LOG_DEBUG, TAST_FMT, uast_function_decl_print(decl));
    Tast_lang_type* rtn_type = NULL;
    try(try_set_lang_type_types(env, &rtn_type, decl->return_type));
    log(LOG_DEBUG, TAST_FMT, tast_lang_type_print(rtn_type));

    Tast_def* struct_def_ = NULL;
    // TODO: think about out of order things
    try(rm_tuple_struct_lookup(&struct_def_, env, serialize_lang_type(rtn_type->lang_type)));

    Ulang_type new_var_lang_type = ulang_type_wrap_regular_const(ulang_type_regular_new(ulang_type_atom_new(tast_unwrap_struct_def(struct_def_)->base.name, 0)));
    Uast_variable_def* new_var_ = uast_variable_def_new(
        tast_get_pos_stmt(assign->lhs),
        new_var_lang_type,
        false,
        util_literal_name_new_prefix("tuple_assign_lhs")
    );
    try(usym_tbl_add(&vec_at(&env->ancesters, 0)->usymbol_table, uast_wrap_variable_def(new_var_)));
    Tast_variable_def* new_var = NULL;
    try(try_set_variable_def_types(env, &new_var, new_var_, false));
    try(sym_tbl_add(&vec_at(&env->ancesters, 0)->symbol_table, tast_wrap_variable_def(new_var)));
    Tast_assignment* new_assign = tast_assignment_new(
        dest->pos,
        tast_wrap_def(tast_wrap_variable_def(new_var)),
        tast_wrap_function_call(src)
    );
    vec_append(&a_main, &new_children, tast_wrap_assignment(new_assign));

    for (size_t idx = 0; idx < dest->members.info.count; idx++) {
        try(idx < tast_unwrap_struct_def(struct_def_)->base.members.info.count);
        Tast_variable_def* curr_memb_def = vec_at(&tast_unwrap_struct_def(struct_def_)->base.members, idx);

        Uast_member_access_untyped* curr_src_ = uast_member_access_untyped_new(
            src->pos,
            curr_memb_def->name,
            uast_wrap_symbol_untyped(uast_symbol_untyped_new(dest->pos, new_var->name))
        );
        Tast_stmt* curr_src = NULL;
        try(try_set_member_access_types(env, &curr_src, curr_src_));

        Tast_expr* curr_dest = vec_at(&dest->members, idx);
        Tast_assignment* curr_assign = tast_assignment_new(
            dest->pos,
            tast_wrap_expr(curr_dest),
            tast_unwrap_expr(curr_src)
        );
        vec_append(&a_main, &new_children, tast_wrap_assignment(curr_assign));
    }

    src->lang_type = lang_type_from_ulang_type(env, new_var_lang_type);

    //log(LOG_DEBUG, TAST_FMT, tast_block_print(tast_block_new(assign->pos, false, new_children, (Symbol_collection) {0}, assign->pos)));
    //todo();
    return tast_wrap_block(tast_block_new(assign->pos, false, new_children, (Symbol_collection) {0}, assign->pos));
}

static Tast_function_def* rm_tuple_function_def_new(Env* env, Tast_function_decl* decl) {
    Tast_block* body = tast_block_new(decl->pos, false, (Tast_stmt_vec) {0}, (Symbol_collection) {0}, decl->pos);

    Tast_def* struct_def_ = NULL;
    try(symbol_lookup(&struct_def_, env, lang_type_get_str(decl->return_type->lang_type)));
    Tast_struct_def* struct_def = tast_unwrap_struct_def(struct_def_);

    vec_append(&a_main, &env->ancesters, &body->symbol_collection);

    Str_view new_var_name = util_literal_name_new_prefix("struct_var_in_tuple_function");
    Uast_variable_def* new_var_untyped = uast_variable_def_new(
        decl->pos,
        ulang_type_wrap_regular_const(ulang_type_regular_new(ulang_type_atom_new(struct_def->base.name, 0))),
        false,
        new_var_name
    );
    //Tast_variable_def* new_var = tast_variable_def_new(
    //    decl->pos,
    //    lang_type_new_from_strv(struct_def->base.name, 0),
    //    false,
    //    new_var_name
    //);
    Tast_variable_def* new_var = NULL;
    try(usym_tbl_add(&vec_at(&env->ancesters, 0)->usymbol_table, uast_wrap_variable_def(new_var_untyped)));
    try(try_set_variable_def_types(env, &new_var, new_var_untyped, true));
    vec_append(&a_main, &body->children, tast_wrap_def(tast_wrap_variable_def(new_var)));

    for (size_t idx = 0; idx < decl->params->params.info.count; idx++) {
        Uast_member_access_untyped* lhs_untyped = uast_member_access_untyped_new(
            decl->pos,
            vec_at(&struct_def->base.members, idx)->name,
            uast_wrap_symbol_untyped(uast_symbol_untyped_new(decl->pos, new_var->name))
        );
        env->parent_of = PARENT_OF_ASSIGN_RHS;
        Tast_stmt* lhs = NULL;
        try(try_set_member_access_types(env, &lhs, lhs_untyped));
        env->parent_of = PARENT_OF_NONE;

        Uast_symbol_untyped* rhs_untyped = uast_symbol_untyped_new(decl->pos, vec_at(&decl->params->params, idx)->name);
        Tast_expr* rhs = NULL;
        try(try_set_symbol_type(env, &rhs, rhs_untyped));

        Tast_assignment* assign = tast_assignment_new(decl->pos, lhs, rhs);
        vec_append(&a_main, &body->children, tast_wrap_assignment(assign));

    }

    Tast_function_def* new_def = tast_function_def_new(decl->pos, decl, body);

    Uast_symbol_untyped* rtn_child_ = uast_symbol_untyped_new(decl->pos, new_var->name);
    Tast_expr* rtn_child = NULL;
    try(try_set_symbol_type(env, &rtn_child, rtn_child_));

    Tast_return* new_rtn = tast_return_new(decl->pos, rtn_child, true);
    vec_append(&a_main, &body->children, tast_wrap_return(new_rtn));
    
    vec_rem_last(&env->ancesters);

    return new_def;
}

static Tast_expr* rm_tuple_generic_assign_struct_literal_child(
    Env* env,
    Tast_expr* rhs,
    Lang_type lhs_lang_type,
    Pos assign_pos
) {
    Tast_def* def = NULL;
    try(symbol_lookup(&def, env, env->name_parent_function));
    Tast_function_decl* fun_decl = tast_unwrap_function_decl(def);

    try(fun_decl->return_type->lang_type.type != LANG_TYPE_TUPLE);
    //Tast_variable_def* to_rtn = tast_variable_def_new(
    //    rtn->pos,
    //    vec_at(&fun_decl->return_type->lang_type, 0),
    //    false,
    //    util_literal_name_new_prefix("tuple_to_return")
    //);
    //
    Tast_expr_vec members = {0};
    switch (rhs->type) {
        case TAST_STRUCT_LITERAL:
            members = tast_unwrap_struct_literal(rhs)->members;
            break;
        case TAST_TUPLE:
            members = tast_unwrap_tuple(rhs)->members;
            break;
        case TAST_LITERAL: {
            Tast_sum_lit* sum_lit = tast_unwrap_sum_lit(tast_unwrap_literal(rhs));
            env->parent_of = PARENT_OF_CASE;
            vec_append(&a_main, &members, tast_wrap_literal(tast_wrap_enum_lit(sum_lit->tag)));
            vec_append(&a_main, &members, sum_lit->item);
            env->parent_of = PARENT_OF_NONE;
            break;
        }
        default:
            unreachable("");
    }

    Tast_variable_def_vec struct_def_memb = {0};
    Tast_def* struct_def_ = NULL;
    try(symbol_lookup(&struct_def_, env, lang_type_get_str(lhs_lang_type)));
    switch (struct_def_->type) {
        case TAST_STRUCT_DEF:
            struct_def_memb = tast_unwrap_struct_def(struct_def_)->base.members;
            break;
        default:
            unreachable("");
    }

    Tast_expr_vec new_args = {0};

    log(LOG_DEBUG, STR_VIEW_FMT, tast_function_decl_print(fun_decl));
    //try(symbol_lookup(&struct_def_, env, vec_at(&fun_decl->return_type->lang_type, 0).str));
    Tast_variable_def_vec new_params = {0};
    Str_view new_fun_name = util_literal_name_new_prefix("tuple_function");
    for (size_t idx = 0; idx < members.info.count; idx++) {
        try(idx < struct_def_memb.info.count);
        Tast_variable_def* curr_def = vec_at(&struct_def_memb, idx);
        //Tast_member_access_typed* access = 

        Uast_variable_def* new_param_ = uast_variable_def_new(
            assign_pos,
            lang_type_to_ulang_type(curr_def->lang_type),
            false,
            util_literal_name_new_prefix("tuple_function_param")
        );
        try(usym_tbl_add(&vec_at(&env->ancesters, 0)->usymbol_table, uast_wrap_variable_def(new_param_)));
        Tast_variable_def* new_param = NULL;
        try(try_set_variable_def_types(env, &new_param, new_param_, false));
        try(sym_tbl_add(&vec_at(&env->ancesters, 0)->symbol_table, tast_wrap_variable_def(new_param)));
        vec_append(&a_main, &new_params, new_param);
        log(LOG_DEBUG, STR_VIEW_FMT, tast_variable_def_print(new_param));

        Tast_expr* new_memb = rm_tuple_generic_assignment_rhs(
            env,
            vec_at(&members, idx),
            curr_def->lang_type,
            tast_get_pos_expr(vec_at(&members, idx))
        );

        vec_append(&a_main, &new_args, new_memb);
    }
    rm_tuple_stru_tbl_add(&vec_at(&env->ancesters, 0)->rm_tuple_struct_table, struct_def_);
    log(LOG_DEBUG, STR_VIEW_FMT"\n", str_view_print(serialize_def(struct_def_)));
    //log(LOG_DEBUG, STR_VIEW_FMT"\n", str_view_print(rm_tuple_struct_get_name_from_return_type(struct_def)));
    try(rm_tuple_struct_lookup(&struct_def_, env, serialize_def(struct_def_)));

    Tast_function_params* new_fun_params = tast_function_params_new(assign_pos, new_params);
    Tast_function_decl* new_fun_decl = tast_function_decl_new(
        assign_pos,
        new_fun_params,
        tast_lang_type_new(
            assign_pos,
            lang_type_wrap_struct_const(lang_type_struct_new(lang_type_atom_new(tast_unwrap_struct_def(struct_def_)->base.name, 0)))
        ),
        new_fun_name
    );
    Tast_function_def* new_fun_def = rm_tuple_function_def_new(env, new_fun_decl);
    try(sym_tbl_add(&vec_at(&env->ancesters, 0)->symbol_table, tast_wrap_function_def(new_fun_def)));
    vec_append(&a_main, &env->extra_functions, new_fun_def);

    Tast_function_call* fun_call = tast_function_call_new(assign_pos, new_args, new_fun_decl->name, new_fun_decl->return_type->lang_type);
    return tast_wrap_function_call(fun_call);
}

// returns new rhs
static Tast_expr* rm_tuple_generic_assignment_rhs(Env* env, Tast_expr* rhs, Lang_type lhs_lang_type, Pos assign_pos) {
    if (rhs->type == TAST_TUPLE) {
        return rm_tuple_generic_assign_struct_literal_child(env, rhs, lhs_lang_type, assign_pos);
    } else if (rhs->type == TAST_STRUCT_LITERAL) {
        return rm_tuple_generic_assign_struct_literal_child(env, rhs, lhs_lang_type, assign_pos);
    } else if (rhs->type == TAST_LITERAL && tast_unwrap_literal(rhs)->type == TAST_SUM_LIT) {
        return rm_tuple_generic_assign_struct_literal_child(env, rhs, lhs_lang_type, assign_pos);
    } else {
        return rhs;
    }
}

static Tast_return* rm_tuple_return(Env* env, Tast_return* rtn) {
    if (env->name_parent_function.count < 1) {
        return rtn;
    }

    Tast_def* def = NULL;
    try(symbol_lookup(&def, env, env->name_parent_function));
    Tast_function_decl* fun_decl = tast_unwrap_function_decl(def);

    Tast_def* struct_def_ = NULL;
    if (!symbol_lookup(&struct_def_, env, lang_type_get_str(fun_decl->return_type->lang_type))) {
        return rtn;
    }

    rtn->child = rm_tuple_generic_assignment_rhs(
        env,
        rtn->child,
        fun_decl->return_type->lang_type,
        rtn->pos
    );
    return rtn;
}

static Tast_if* rm_tuple_if(Env* env, Tast_if* lang_if) {
    lang_if->body = rm_tuple_block(env, lang_if->body);
    return lang_if;
}

static Tast_if_else_chain* rm_tuple_if_else_chain(Env* env, Tast_if_else_chain* if_else) {
    Tast_if_vec new_ifs = {0};
    for (size_t idx = 0; idx < if_else->tasts.info.count; idx++) {
        vec_append(&a_main, &new_ifs, rm_tuple_if(env, vec_at(&if_else->tasts, idx)));
    }

    if_else->tasts = new_ifs;
    return if_else;
}

static Tast_function_def* rm_tuple_function_def(Env* env, Tast_function_def* def) {
    Str_view old_fun_name = env->name_parent_function;

    env->name_parent_function = def->decl->name;

    Lang_type rtn_type = def->decl->return_type->lang_type;
    env->rm_tuple_parent_fn_lang_type = rtn_type;

    vec_append(&a_main, &env->ancesters, &def->body->symbol_collection);

    if (rtn_type.type == LANG_TYPE_TUPLE) {
        Uast_variable_def_vec members = {0};

        for (size_t idx = 0; idx < lang_type_unwrap_tuple_const(rtn_type).lang_types.info.count; idx++) {
            Uast_variable_def* memb = uast_variable_def_new(
                def->decl->return_type->pos,
                lang_type_to_ulang_type(vec_at_const(lang_type_unwrap_tuple_const(rtn_type).lang_types, idx)),
                false,
                util_literal_name_new_prefix("tuple_struct_member")
            );
            vec_append(&a_main, &members, memb);
        }

        Ustruct_def_base base = {
            .members = members,
            .name = util_literal_name_new_prefix("tuple_struct")
        };
        Uast_struct_def* struct_def_ = uast_struct_def_new(def->decl->return_type->pos, base);
        Tast_struct_def* struct_def = NULL;
        try(usym_tbl_add(&vec_at(&env->ancesters, 0)->usymbol_table, uast_wrap_struct_def(struct_def_)));
        try(try_set_struct_def_types(env, &struct_def, struct_def_));
        vec_append(&a_main, &env->extra_structs, struct_def);
        log(LOG_DEBUG, STR_VIEW_FMT, tast_struct_def_print(struct_def));
        def->decl->return_type->lang_type = lang_type_wrap_struct_const(lang_type_struct_new(lang_type_atom_new(base.name, 0)));
        try(sym_tbl_add(&vec_at(&env->ancesters, 0)->symbol_table, tast_wrap_struct_def(struct_def)));
    }
    
    vec_rem_last(&env->ancesters);

    def->body = rm_tuple_block(env, def->body);

    env->name_parent_function = old_fun_name;

    return def;
}

static Tast_def* rm_tuple_def(Env* env, Tast_def* def) {
    switch (def->type) {
        case TAST_FUNCTION_DEF:
            return tast_wrap_function_def(rm_tuple_function_def(env, tast_unwrap_function_def(def)));
        case TAST_VARIABLE_DEF:
            return def;
        case TAST_STRUCT_DEF:
            return def;
        case TAST_RAW_UNION_DEF:
            return def;
        case TAST_ENUM_DEF:
            return def;
        case TAST_SUM_DEF:
            return def;
        case TAST_PRIMITIVE_DEF:
            return def;
        case TAST_LITERAL_DEF:
            return def;
        case TAST_FUNCTION_DECL:
            // TODO: do function decl if tuple is contained within it
            return def;
    }
    unreachable("");
}

static Tast_function_call* rm_tuple_function_call(Env* env, Tast_function_call* fun_call) {
    Uast_def* fun_decl_ = NULL;
    try(usymbol_lookup(&fun_decl_, env, fun_call->name));
    Uast_function_decl* fun_decl = uast_unwrap_function_decl(fun_decl_);

    for (size_t idx = 0; idx < MIN(fun_decl->params->params.info.count, fun_call->args.info.count); idx++) {
        Tast_expr** curr = vec_at_ref(&fun_call->args, idx);
        *curr = rm_tuple_generic_assignment_rhs(
            env,
            *curr,
            lang_type_from_ulang_type(env, vec_at(&fun_decl->params->params, idx)->lang_type),
            tast_get_pos_expr(*curr)
        );
    }
    return fun_call;
}

static Tast_expr* rm_tuple_sum_rhs(Env* env, Lang_type lhs_lang_type, Tast_sum_lit* rhs, Pos assign_pos) {
    return rm_tuple_generic_assignment_rhs(env, tast_wrap_literal(tast_wrap_sum_lit(rhs)), lhs_lang_type, assign_pos);
}

static Tast_expr* rm_tuple_literal_rhs(Env* env, Lang_type lhs_lang_type, Tast_literal* rhs, Pos assign_pos) {
    switch (rhs->type) {
        case TAST_NUMBER:
            return tast_wrap_literal(rhs);
        case TAST_STRING:
            return tast_wrap_literal(rhs);
        case TAST_VOID:
            return tast_wrap_literal(rhs);
        case TAST_ENUM_LIT:
            return tast_wrap_literal(rhs);
        case TAST_CHAR:
            return tast_wrap_literal(rhs);
        case TAST_SUM_LIT:
            return rm_tuple_sum_rhs(env, lhs_lang_type, tast_unwrap_sum_lit(rhs), assign_pos);
    }
    unreachable("");
}

static Tast_expr* rm_tuple_expr_rhs(Env* env, Lang_type lhs_lang_type, Tast_expr* rhs, Pos assign_pos) {
    switch (rhs->type) {
        case TAST_OPERATOR:
            return rhs;
        case TAST_SYMBOL_TYPED:
            return rhs;
        case TAST_MEMBER_ACCESS_TYPED:
            return rhs;
        case TAST_INDEX_TYPED:
            return rhs;
        case TAST_LITERAL:
            return rm_tuple_literal_rhs(env, lhs_lang_type, tast_unwrap_literal(rhs), assign_pos);
        case TAST_STRUCT_LITERAL:
            return rm_tuple_generic_assignment_rhs(env, rhs, lhs_lang_type, assign_pos);
        case TAST_TUPLE:
            return rm_tuple_generic_assignment_rhs(env, rhs, lhs_lang_type, assign_pos);
        case TAST_FUNCTION_CALL:
            return tast_wrap_function_call(rm_tuple_function_call(env, tast_unwrap_function_call(rhs)));
        case TAST_SUM_CALLEE:
            todo();
    }
    unreachable("");
}

static Tast_binary* rm_tuple_binary_not_in_assignment(Env* env, Tast_binary* oper) {
    oper->lhs = rm_tuple_expr_not_in_assignment(env, oper->lhs);
    oper->rhs = rm_tuple_expr_not_in_assignment(env, oper->rhs);
    return oper;
}

static Tast_unary* rm_tuple_unary_not_in_assignment(Env* env, Tast_unary* unary) {
    unary->child = rm_tuple_expr_not_in_assignment(env, unary->child);
    return unary;
}

static Tast_operator* rm_tuple_operator_not_in_assignment(Env* env, Tast_operator* oper) {
    switch (oper->type) {
        case TAST_UNARY:
            return tast_wrap_unary(rm_tuple_unary_not_in_assignment(env, tast_unwrap_unary(oper)));
        case TAST_BINARY:
            return tast_wrap_binary(rm_tuple_binary_not_in_assignment(env, tast_unwrap_binary(oper)));
    }
}

static Tast_expr* rm_tuple_expr_not_in_assignment(Env* env, Tast_expr* expr) {
    switch (expr->type) {
        case TAST_OPERATOR:
            return tast_wrap_operator(rm_tuple_operator_not_in_assignment(env, tast_unwrap_operator(expr)));
        case TAST_SYMBOL_TYPED:
            unreachable("");
        case TAST_MEMBER_ACCESS_TYPED:
            unreachable("");
        case TAST_INDEX_TYPED:
            unreachable("");
        case TAST_LITERAL:
            unreachable("");
        case TAST_STRUCT_LITERAL:
            unreachable("");
        case TAST_FUNCTION_CALL:
            return tast_wrap_function_call(rm_tuple_function_call(env, tast_unwrap_function_call(expr)));
        case TAST_TUPLE:
            unreachable("");
        case TAST_SUM_CALLEE:
            unreachable("");
    }
    unreachable("");
}

static Tast_stmt* rm_tuple_stmt(Env* env, Tast_stmt* stmt) {
    switch (stmt->type) {
        case TAST_BLOCK:
            return tast_wrap_block(rm_tuple_block(env, tast_unwrap_block(stmt)));
        case TAST_EXPR:
            return tast_wrap_expr(rm_tuple_expr_not_in_assignment(env, tast_unwrap_expr(stmt)));
        case TAST_FOR_RANGE:
            return tast_wrap_for_range(rm_tuple_for_range(env, tast_unwrap_for_range(stmt)));
        case TAST_FOR_WITH_COND:
            return tast_wrap_for_with_cond(rm_tuple_for_with_cond(env, tast_unwrap_for_with_cond(stmt)));
        case TAST_IF_ELSE_CHAIN:
            return tast_wrap_if_else_chain(rm_tuple_if_else_chain(env, tast_unwrap_if_else_chain(stmt)));
        case TAST_RETURN:
            return tast_wrap_return(rm_tuple_return(env, tast_unwrap_return(stmt)));
        case TAST_BREAK:
            return stmt;
        case TAST_CONTINUE:
            return stmt;
        case TAST_ASSIGNMENT:
            return rm_tuple_assignment(env, tast_unwrap_assignment(stmt));
        case TAST_DEF:
            return tast_wrap_def(rm_tuple_def(env, tast_unwrap_def(stmt)));
    }
    unreachable("");
}

static Tast* rm_tuple(Env* env, Tast* tast) {
    switch (tast->type) {
        case TAST_STMT:
            return tast_wrap_stmt(rm_tuple_stmt(env, tast_unwrap_stmt(tast)));
        case TAST_LANG_TYPE:
            return tast;
        case TAST_FOR_LOWER_BOUND:
            return tast;
        case TAST_FUNCTION_PARAMS:
            return tast;
        case TAST_FOR_UPPER_BOUND:
            return tast;
        case TAST_CONDITION:
            return tast;
        case TAST_IF:
            return tast_wrap_if(rm_tuple_if(env, tast_unwrap_if(tast)));
    }
    unreachable("");
}

static Tast_block* rm_tuple_block(Env* env, Tast_block* block) {
    Tast_stmt_vec new_children = {0};

    vec_append(&a_main, &env->ancesters, &block->symbol_collection);
    for (size_t idx = 0; idx < block->children.info.count; idx++) {
        Tast_stmt* curr = vec_at(&block->children, idx);

        vec_append(&a_main, &new_children, rm_tuple_stmt(env, curr));
        
    }
    vec_rem_last(&env->ancesters);

    block->children = new_children;
    return block;
}

Tast_block* remove_tuples(Env* env, Tast_block* root) {
    Tast_block* new_block = rm_tuple_block(env, root);
    // TODO: use faster algorithm for inserting extra structs and functions
    for (size_t idx = 0; idx < env->extra_functions.info.count; idx++) {
        assert(vec_at(&env->extra_functions, idx));
        vec_insert(&a_main, &new_block->children, 0, tast_wrap_def(tast_wrap_function_def(vec_at(&env->extra_functions, idx))));
    }
    for (size_t idx = 0; idx < env->extra_structs.info.count; idx++) {
        assert(vec_at(&env->extra_structs, idx));
        vec_insert(&a_main, &new_block->children, 0, tast_wrap_def(tast_wrap_struct_def(vec_at(&env->extra_structs, idx))));
    }
    return new_block;
}
