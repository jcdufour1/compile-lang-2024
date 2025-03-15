#include <generic_sub.h>
#include <ulang_type_serialize.h>
#include <ulang_type.h>
#include <ulang_type_print.h>
#include <ulang_type_clone.h>
#include <resolve_generics.h>
#include <uast_clone.h>
#include <uast_utils.h>
#include <env.h>

void generic_sub_return(Uast_return* rtn, Str_view gen_param, Ulang_type gen_arg) {
    log(LOG_DEBUG, TAST_FMT, uast_return_print(rtn));
    (void) rtn;
    (void) gen_param;
    (void) gen_arg;
}

void generic_sub_param(Uast_param* def, Str_view gen_param, Ulang_type gen_arg) {
    generic_sub_variable_def(def->base, gen_param, gen_arg);
}

void generic_sub_lang_type_regular(
    Ulang_type* new_lang_type,
    Ulang_type_regular lang_type,
    Str_view gen_param,
    Ulang_type gen_arg
) {
    if (str_view_is_equal(gen_param, lang_type.atom.str)) {
        *new_lang_type = ulang_type_clone(gen_arg);
    } else {
        *new_lang_type = ulang_type_clone(ulang_type_regular_const_wrap(lang_type));
    }
    return;
}

void generic_sub_lang_type_reg_generic(
    Env* env,
    Ulang_type* new_lang_type,
    Ulang_type_reg_generic lang_type,
    Str_view gen_param,
    Ulang_type gen_arg
) {
    Ulang_type_reg_generic new_reg = ulang_type_reg_generic_clone(lang_type);

    for (size_t idx = 0; idx < new_reg.generic_args.info.count; idx++) {
        generic_sub_lang_type(
            env,
            vec_at_ref(&new_reg.generic_args, idx),
            vec_at(&new_reg.generic_args, idx),
            gen_param,
            ulang_type_clone(gen_arg)
        );
    }

    resolve_generics_ulang_type_reg_generic(new_lang_type, env, new_reg);
    log(LOG_DEBUG, TAST_FMT, ulang_type_print(LANG_TYPE_MODE_LOG, ulang_type_reg_generic_const_wrap(lang_type)));
    log(LOG_DEBUG, TAST_FMT, ulang_type_print(LANG_TYPE_MODE_LOG, *new_lang_type));
}

void generic_sub_lang_type(
    Env* env,
    Ulang_type* new_lang_type,
    Ulang_type lang_type,
    Str_view gen_param,
    Ulang_type gen_arg
) {
    switch (lang_type.type) {
        case ULANG_TYPE_REGULAR:
            generic_sub_lang_type_regular(
                new_lang_type,
                ulang_type_regular_const_unwrap(lang_type),
                gen_param,
                gen_arg
            );
            return;
        case ULANG_TYPE_REG_GENERIC: {
            generic_sub_lang_type_reg_generic(
                env,
                new_lang_type,
                ulang_type_reg_generic_const_unwrap(lang_type),
                gen_param,
                gen_arg
            );
            return;
        }
        case ULANG_TYPE_FN:
            todo();
        case ULANG_TYPE_TUPLE:
            todo();
    }
    unreachable("");

    //log(
    //    LOG_DEBUG, "gen_str: "STR_VIEW_FMT"    rtn_str: "STR_VIEW_FMT"\n",
    //    str_view_print(gen_param), str_view_print(lang_type_str)
    //);

    //log(
    //    LOG_DEBUG, "new_lang_type: "STR_VIEW_FMT"\n",
    //    ulang_type_print(*new_lang_type)
    //);
}

void generic_sub_variable_def(Uast_variable_def* def, Str_view gen_param, Ulang_type gen_arg) {
    Str_view reg_param = ulang_type_regular_const_unwrap(def->lang_type).atom.str;
    if (str_view_is_equal(gen_param, reg_param)) {
        def->lang_type = ulang_type_clone(gen_arg);
    }
}

void generic_sub_struct_def_base(Ustruct_def_base* base, Str_view gen_param, Ulang_type gen_arg) {
    for (size_t idx = 0; idx < base->members.info.count; idx++) {
        Str_view memb = ulang_type_regular_const_unwrap(vec_at(&base->members, idx)->lang_type).atom.str;
        if (str_view_is_equal(gen_param, memb)) {
            vec_at(&base->members, idx)->lang_type = ulang_type_clone(gen_arg);
        }
    }
}

void generic_sub_def(Uast_def* def, Str_view gen_param, Ulang_type gen_arg) {
    switch (def->type) {
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
    }
    unreachable("");
}

void generic_sub_stmt(Env* env, Uast_stmt* stmt, Str_view gen_param, Ulang_type gen_arg) {
    switch (stmt->type) {
        case UAST_BLOCK:
            generic_sub_block(env, uast_block_unwrap(stmt), gen_param, gen_arg);
            return;
        case UAST_EXPR:
            generic_sub_expr(uast_expr_unwrap(stmt), gen_param, gen_arg);
            return;
        case UAST_DEF:
            generic_sub_def(uast_def_unwrap(stmt), gen_param, gen_arg);
            return;
        case UAST_FOR_WITH_COND:
            todo();
        case UAST_BREAK:
            todo();
        case UAST_CONTINUE:
            todo();
        case UAST_ASSIGNMENT:
            todo();
        case UAST_RETURN:
            generic_sub_return(uast_return_unwrap(stmt), gen_param, gen_arg);
            return;
        case UAST_IF_ELSE_CHAIN:
            todo();
        case UAST_SWITCH:
            todo();
    }
    unreachable("");
}

void generic_sub_block(Env* env, Uast_block* block, Str_view gen_param, Ulang_type gen_arg) {
    (void) env;
    Usymbol_table tbl = block->symbol_collection.usymbol_table;
    for (size_t idx = 0; idx < tbl.capacity; idx++) {
        if (tbl.table_tasts[idx].status != SYM_TBL_OCCUPIED) {
            continue;
        }


        log(LOG_DEBUG, TAST_FMT, uast_variable_def_print(uast_variable_def_unwrap(tbl.table_tasts[idx].tast)));
        log(LOG_DEBUG, LANG_TYPE_FMT, ulang_type_print(LANG_TYPE_MODE_LOG, uast_variable_def_unwrap(tbl.table_tasts[idx].tast)->lang_type));
        //log(LOG_DEBUG, "thing 9870: %p\n", ulang_type_regular_const_unwrap(uast_variable_def_unwrap(tbl.table_tasts[idx].tast)->lang_type));
        
        log(LOG_DEBUG, TAST_FMT, uast_def_print(tbl.table_tasts[idx].tast));
        log(LOG_DEBUG, "thing 9877: %p\n", (void*)tbl.table_tasts[idx].tast);

        //log(LOG_DEBUG, "old_block: %p  new_block: %p\n", (void*)def->body, (void*)new_block);
        generic_sub_def(tbl.table_tasts[idx].tast, gen_param, gen_arg);
    }

    for (size_t idx = 0; idx < block->children.info.count; idx++) {
        generic_sub_stmt(env, vec_at(&block->children, idx), gen_param, gen_arg);
    }
}

void generic_sub_expr(Uast_expr* expr, Str_view gen_param, Ulang_type gen_arg) {
    switch (expr->type) {
        case UAST_OPERATOR:
            generic_sub_operator(uast_operator_unwrap(expr), gen_param, gen_arg);
            return;
        case UAST_UNKNOWN:
            return;
        case UAST_SYMBOL:
            return;
        case UAST_MEMBER_ACCESS:
            todo();
        case UAST_INDEX:
            todo();
        case UAST_FUNCTION_CALL:
            todo();
        case UAST_LITERAL:
            todo();
        case UAST_STRUCT_LITERAL:
            todo();
        case UAST_TUPLE:
            todo();
        case UAST_SUM_ACCESS:
            todo();
        case UAST_SUM_GET_TAG:
            todo();
    }
    unreachable("");
}

void generic_sub_operator(Uast_operator* oper, Str_view gen_param, Ulang_type gen_arg) {
    switch (oper->type) {
        case UAST_BINARY:
            generic_sub_binary(uast_binary_unwrap(oper), gen_param, gen_arg);
            return;
        case UAST_UNARY:
            generic_sub_unary(uast_unary_unwrap(oper), gen_param, gen_arg);
            return;
    }
}

void generic_sub_binary(Uast_binary* bin, Str_view gen_param, Ulang_type gen_arg) {
    (void) bin;
    (void) gen_param;
    (void) gen_arg;
    switch (bin->token_type) {
        case BINARY_SINGLE_EQUAL:
            return;
        case BINARY_SUB:
            todo();
        case BINARY_ADD:
            todo();
        case BINARY_MULTIPLY:
            todo();
        case BINARY_DIVIDE:
            todo();
        case BINARY_MODULO:
            todo();
        case BINARY_LESS_THAN:
            todo();
        case BINARY_LESS_OR_EQUAL:
            todo();
        case BINARY_GREATER_OR_EQUAL:
            todo();
        case BINARY_GREATER_THAN:
            todo();
        case BINARY_DOUBLE_EQUAL:
            todo();
        case BINARY_NOT_EQUAL:
            todo();
        case BINARY_BITWISE_XOR:
            todo();
        case BINARY_BITWISE_AND:
            todo();
        case BINARY_BITWISE_OR:
            todo();
        case BINARY_LOGICAL_AND:
            todo();
        case BINARY_LOGICAL_OR:
            todo();
        case BINARY_SHIFT_LEFT:
            todo();
        case BINARY_SHIFT_RIGHT:
            todo();
    }
    unreachable("");
}

void generic_sub_unary(Uast_unary* unary, Str_view gen_param, Ulang_type gen_arg) {
    (void) unary;
    (void) gen_param;
    (void) gen_arg;
}
