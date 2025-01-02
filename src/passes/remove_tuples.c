
#include "passes.h"
#include <util.h>
#include <llvm_utils.h>
#include <tast_utils.h>
#include <uast_utils.h>
#include <parser_utils.h>
#include <type_checking.h>

static Tast_block* rm_tuple_block(Env* env, Tast_block* block);

static Tast_for_range* rm_tuple_for_range(Env* env, Tast_for_range* lang_for);

static Tast_for_with_cond* rm_tuple_for_with_cond(Env* env, Tast_for_with_cond* lang_for);

static Tast* rm_tuple_assignment(Env* env, Tast_assignment* assign);

static Tast_block* rm_tuple_return(Env* env, Tast_return* rtn);

static Tast_if* rm_tuple_if(Env* env, Tast_if* assign);

static Tast_if_else_chain* rm_tuple_if_else_chain(Env* env, Tast_if_else_chain* if_else);

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

static Tast* rm_tuple_assignment(Env* env, Tast_assignment* assign) {
    (void) env;

    if (tast_get_lang_types(assign->lhs).info.count < 2 && tast_get_lang_types_expr(assign->rhs).info.count < 2) {
        return tast_wrap_assignment(assign);
    }

    Tast_tuple* dest = tast_unwrap_tuple(tast_unwrap_expr(assign->lhs));
    Tast_function_call* src = tast_unwrap_function_call(assign->rhs);

    Tast_expr_vec new_args = {0};

    for (size_t idx = 0; idx < dest->members.info.count; idx++) {
        Tast_expr* curr_dest = vec_at(&dest->members, idx);
        Tast_unary* new_dest = tast_unary_new(tast_get_pos_expr(curr_dest), curr_dest, TOKEN_REFER, (Lang_type) {0});
        vec_append(&a_main, &new_args, tast_wrap_operator(tast_wrap_unary(new_dest)));
    }

    vec_extend(&a_main, &new_args, &src->args);
    src->args = new_args;
    src->lang_type = lang_type_vec_from_lang_type(lang_type_new_from_cstr("void", 0));

    return tast_wrap_expr(tast_wrap_function_call(src));
}

static Tast_block* rm_tuple_return(Env* env, Tast_return* rtn) {
    Tast_vec new_children = {0};

    if (rtn->child->type != TAST_TUPLE) {
        vec_append(&a_main, &new_children, tast_wrap_return(rtn));
        return tast_block_new(rtn->pos, false, new_children, (Symbol_collection) {0}, rtn->pos);
    }

    Tast_def* def = NULL;
    try(symbol_lookup(&def, env, env->name_parent_function));
    Tast_function_decl* fun_decl = tast_unwrap_function_decl(def);

    Tast_tuple* old_tuple = tast_unwrap_tuple(rtn->child);

    for (size_t idx = 0; idx < old_tuple->members.info.count; idx++) {
        Tast_variable_def* curr_param = vec_at(&fun_decl->params->params, idx);

        Uast_symbol_untyped* curr_dest_inner = uast_symbol_untyped_new(curr_param->pos, curr_param->name);
        Uast_unary* curr_dest_ = uast_unary_new(
            curr_param->pos,
            uast_wrap_symbol_untyped(curr_dest_inner),
            TOKEN_DEREF,
            (Lang_type) {0}
        );
        Tast_expr* curr_dest = NULL;
        try(try_set_unary_types(env, &curr_dest, curr_dest_));

        Tast_expr* curr_src = vec_at(&old_tuple->members, idx);

        Tast_assignment* new_assign = tast_assignment_new(
            tast_get_pos_expr(curr_src),
            tast_wrap_expr(curr_dest),
            curr_src
        );
        vec_append(&a_main, &new_children, tast_wrap_assignment(new_assign));
    }

    vec_append(&a_main, &new_children, tast_wrap_return(tast_return_new(
        rtn->pos,
        tast_wrap_literal(tast_wrap_void(tast_void_new(rtn->pos))),
        true
    )));
    return tast_block_new(rtn->pos, false, new_children, (Symbol_collection) {0}, rtn->pos);
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

    Lang_type_vec rtn_type = def->decl->return_type->lang_type;
    env->rm_tuple_parent_fn_lang_type = rtn_type;

    vec_append(&a_main, &env->ancesters, &def->body->symbol_collection);

    if (def->decl->return_type->lang_type.info.count > 1) {
        Tast_variable_def_vec new_params = {0};
        for (size_t idx = 0; idx < rtn_type.info.count; idx++) {
            Lang_type curr = vec_at(&rtn_type, idx);
            curr.pointer_depth++;

            Str_view new_name = util_literal_name_new_prefix("tuple_param");
            Tast_variable_def* new_var = tast_variable_def_new(
                def->decl->return_type->pos, curr, false, new_name
            );
            Uast_variable_def* new_var_untyped = uast_variable_def_new(
                def->decl->return_type->pos, curr, false, new_name
            );
            try(usymbol_add(env, uast_wrap_variable_def(new_var_untyped)));
            try(symbol_add(env, tast_wrap_variable_def(new_var)));
            vec_append(&a_main, &new_params, new_var);
        }

        vec_extend(&a_main, &new_params, &def->decl->params->params);
        def->decl->params->params = new_params;

        def->decl->return_type->lang_type = lang_type_vec_from_lang_type(lang_type_new_from_cstr("void", 0));
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

static Tast* rm_tuple(Env* env, Tast* tast) {
    switch (tast->type) {
        case TAST_EXPR:
            return tast;
        case TAST_BLOCK:
            return tast_wrap_block(rm_tuple_block(env, tast_unwrap_block(tast)));
        case TAST_FUNCTION_PARAMS:
            return tast;
        case TAST_LANG_TYPE:
            return tast;
        case TAST_FOR_LOWER_BOUND:
            return tast;
        case TAST_FOR_UPPER_BOUND:
            return tast;
        case TAST_DEF:
            return tast_wrap_def(rm_tuple_def(env, tast_unwrap_def(tast)));
        case TAST_CONDITION:
            return tast;
        case TAST_FOR_RANGE:
            return tast_wrap_for_range(rm_tuple_for_range(env, tast_unwrap_for_range(tast)));
        case TAST_FOR_WITH_COND:
            return tast_wrap_for_with_cond(rm_tuple_for_with_cond(env, tast_unwrap_for_with_cond(tast)));
        case TAST_BREAK:
            return tast;
        case TAST_CONTINUE:
            return tast;
        case TAST_ASSIGNMENT:
            return rm_tuple_assignment(env, tast_unwrap_assignment(tast));
        case TAST_IF:
            return tast_wrap_if(rm_tuple_if(env, tast_unwrap_if(tast)));
        case TAST_RETURN:
            return tast_wrap_block(rm_tuple_return(env, tast_unwrap_return(tast)));
        case TAST_IF_ELSE_CHAIN:
            return tast_wrap_if_else_chain(rm_tuple_if_else_chain(env, tast_unwrap_if_else_chain(tast)));
    }
    unreachable("");
}

static Tast_block* rm_tuple_block(Env* env, Tast_block* block) {
    Tast_vec new_children = {0};

    vec_append(&a_main, &env->ancesters, &block->symbol_collection);
    for (size_t idx = 0; idx < block->children.info.count; idx++) {
        Tast* curr = vec_at(&block->children, idx);

        vec_append(&a_main, &new_children, rm_tuple(env, curr));
        
    }
    vec_rem_last(&env->ancesters);

    block->children = new_children;
    return block;
}

Tast_block* remove_tuples(Env* env, Tast_block* root) {
    return rm_tuple_block(env, root);
}
