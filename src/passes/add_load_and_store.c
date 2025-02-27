#include <tast.h>
#include <uast.h>
#include <llvm.h>
#include <tasts.h>
#include <symbol_table.h>
#include <parser_utils.h>
#include <type_checking.h>
#include <log_env.h>
#include <tast_serialize.h>
#include <lang_type_serialize.h>
#include <lang_type_from_ulang_type.h>
#include <token_type_to_operator_type.h>

#include "passes.h"

static bool binary_is_short_circuit(BINARY_TYPE type) {
    switch (type) {
        case BINARY_SUB:
            return false;
        case BINARY_ADD:
            return false;
        case BINARY_MULTIPLY:
            return false;
        case BINARY_DIVIDE:
            return false;
        case BINARY_MODULO:
            return false;
        case BINARY_LESS_THAN:
            return false;
        case BINARY_LESS_OR_EQUAL:
            return false;
        case BINARY_GREATER_OR_EQUAL:
            return false;
        case BINARY_GREATER_THAN:
            return false;
        case BINARY_DOUBLE_EQUAL:
            return false;
        case BINARY_NOT_EQUAL:
            return false;
        case BINARY_BITWISE_XOR:
            return false;
        case BINARY_BITWISE_AND:
            return false;
        case BINARY_BITWISE_OR:
            return false;
        case BINARY_LOGICAL_AND:
            return true;
        case BINARY_LOGICAL_OR:
            return true;
    }
    unreachable("");
}

static Llvm_variable_def* load_variable_def_clone(Tast_variable_def* old_var_def);

static Llvm_alloca* add_load_and_store_alloca_new(Env* env, Llvm_variable_def* var_def) {
    Llvm_alloca* alloca = llvm_alloca_new(var_def->pos, 0, var_def->lang_type, var_def->name_corr_param);
    alloca_add(env, llvm_alloca_wrap(alloca));
    assert(alloca);
    return alloca;
}

static Llvm_function_params* load_function_params_clone(Tast_function_params* old_params) {
    Llvm_function_params* new_params = llvm_function_params_new(old_params->pos, (Llvm_variable_def_vec){0}, 0);

    for (size_t idx = 0; idx < old_params->params.info.count; idx++) {
        vec_append(&a_main, &new_params->params, load_variable_def_clone(vec_at(&old_params->params, idx)));
    }

    return new_params;
}

static Llvm_lang_type* load_lang_type_clone(Tast_lang_type* old_lang_type) {
    Llvm_lang_type* new_lang_type = llvm_lang_type_new(old_lang_type->pos, old_lang_type->lang_type);
    return new_lang_type;
}

static Llvm_function_decl* load_function_decl_clone(Tast_function_decl* old_decl) {
    return llvm_function_decl_new(
        old_decl->pos,
        load_function_params_clone(old_decl->params),
        load_lang_type_clone(old_decl->return_type),
        old_decl->name
    );
}

static Llvm_variable_def* load_variable_def_clone(Tast_variable_def* old_var_def) {
    return llvm_variable_def_new(
        old_var_def->pos,
        old_var_def->lang_type,
        old_var_def->is_variadic,
        0,
        util_literal_name_new(),
        old_var_def->name
    );
}

static Llvm_struct_def* load_struct_def_clone(const Tast_struct_def* old_def) {
    return llvm_struct_def_new(
        old_def->pos,
        old_def->base
    );
}

static Llvm_enum_def* load_enum_def_clone(const Tast_enum_def* old_def) {
    return llvm_enum_def_new(
        old_def->pos,
        old_def->base
    );
}

static Llvm_raw_union_def* load_raw_union_def_clone(const Tast_raw_union_def* old_def) {
    return llvm_raw_union_def_new(
        old_def->pos,
        old_def->base
    );
}

static void do_function_def_alloca_param(Env* env, Llvm_function_params* new_params, Llvm_block* new_block, Llvm_variable_def* param) {
    if (is_struct_like(param->lang_type.type)) {
        param->name_self = param->name_corr_param;
        alloca_add(env, llvm_def_wrap(llvm_variable_def_wrap(param)));
    } else {
        vec_insert(&a_main, &new_block->children, 0, llvm_alloca_wrap(
            add_load_and_store_alloca_new(env, param)
        ));
    }

    vec_append(&a_main, &new_params->params, param);
}

static Llvm_function_params* do_function_def_alloca(
    Env* env,
    Llvm_lang_type** new_rtn_type,
    Tast_lang_type* rtn_type,
    Llvm_block* new_block,
    const Tast_function_params* old_params
) {
    Llvm_function_params* new_params = llvm_function_params_new(
        old_params->pos,
        (Llvm_variable_def_vec) {0},
        0
    );

    bool rtn_is_struct = is_struct_like(rtn_type->lang_type.type);

    Lang_type rtn_lang_type = rtn_type->lang_type;
    if (rtn_is_struct) {
        lang_type_set_pointer_depth(&rtn_lang_type, lang_type_get_pointer_depth(rtn_lang_type) + 1);
        Tast_variable_def* new_def = tast_variable_def_new(
            rtn_type->pos,
            rtn_lang_type,
            false,
            util_literal_name_new_prefix("return_as_parameter")
        );
        Llvm_variable_def* param = load_variable_def_clone(new_def);
        do_function_def_alloca_param(env, new_params, new_block, param);
        *new_rtn_type = llvm_lang_type_new(param->pos, lang_type_void_const_wrap(lang_type_void_new(0)));
        env->struct_rtn_name_parent_function = vec_at(&new_params->params, 0)->name_self;
    } else {
        *new_rtn_type = llvm_lang_type_new(rtn_type->pos, rtn_type->lang_type);
    }

    for (size_t idx = 0; idx < old_params->params.info.count; idx++) {
        Llvm_variable_def* param = load_variable_def_clone(vec_at(&old_params->params, idx));
        do_function_def_alloca_param(env, new_params, new_block, param);
    }

    return new_params;
}

static Llvm_block* load_block(Env* env, Tast_block* old_block);

static Str_view load_expr(Env* env, Llvm_block* new_block, Tast_expr* old_expr);

static Str_view load_ptr(Env* env, Llvm_block* new_block, Tast* old_tast);

static Str_view load_ptr_expr(Env* env, Llvm_block* new_block, Tast_expr* old_expr);

static Str_view load_stmt(Env* env, Llvm_block* new_block, Tast_stmt* old_stmt);

static Str_view load_ptr_stmt(Env* env, Llvm_block* new_block, Tast_stmt* old_stmt);

static Str_view load_tast(Env* env, Llvm_block* new_block, Tast* old_tast);

static Str_view load_operator(
    Env* env,
    Llvm_block* new_block,
    Tast_operator* old_oper
);

static Str_view load_variable_def(
    Env* env,
    Llvm_block* new_block,
    Tast_variable_def* old_var_def
);

static Str_view load_if_else_chain(
    Env* env,
    Llvm_block* new_block,
    Tast_if_else_chain* old_if_else
);

static void add_label(Env* env, Llvm_block* block, Str_view label_name, Pos pos, bool defer_add_sym) {
    Llvm_label* label = llvm_label_new(pos, 0, label_name);
    label->name = label_name;

    if (defer_add_sym) {
        alloca_add_defer(env, llvm_def_wrap(llvm_label_wrap(label)));
    } else {
        try(alloca_add(env, llvm_def_wrap(llvm_label_wrap(label))));
    }

    vec_append(&a_main, &block->children, llvm_def_wrap(llvm_label_wrap(label)));
}

static void if_for_add_cond_goto(
    Env* env,
    Tast_operator* old_oper,
    Llvm_block* block,
    Str_view label_name_if_true,
    Str_view label_name_if_false
) {
    Pos pos = tast_operator_get_pos(old_oper);

    Llvm_cond_goto* cond_goto = llvm_cond_goto_new(
        pos,
        load_operator(env, block, old_oper),
        label_name_if_true,
        label_name_if_false,
        0
    );
    log(LOG_DEBUG, TAST_FMT"\n", llvm_cond_goto_print(cond_goto));

    vec_append(&a_main, &block->children, llvm_cond_goto_wrap(cond_goto));
}

static Tast_assignment* for_loop_cond_var_assign_new(Env* env, Str_view sym_name, Pos pos) {
    Uast_literal* literal = util_uast_literal_new_from_int64_t(1, TOKEN_INT_LITERAL, pos);
    Uast_operator* operator = uast_binary_wrap(uast_binary_new(
        pos,
        uast_symbol_wrap(uast_symbol_new(pos, sym_name)),
        uast_literal_wrap(literal),
        BINARY_ADD
    ));
    return util_assignment_new(
        env,
        uast_expr_wrap(uast_symbol_wrap(uast_symbol_new(pos, sym_name))),
        uast_operator_wrap(operator)
    );
}

static Str_view load_function_call(
    Env* env,
    Llvm_block* new_block,
    Tast_function_call* old_fun_call
) {
    log(LOG_DEBUG, TAST_FMT, tast_function_call_print(old_fun_call));
    bool rtn_is_struct = is_struct_like(old_fun_call->lang_type.type);

    Strv_vec new_args = {0};

    Str_view def_name = {0};
    Lang_type fun_lang_type = old_fun_call->lang_type;
    if (rtn_is_struct) {
        def_name = util_literal_name_new_prefix("result_fun_call");
        Tast_variable_def* def = tast_variable_def_new(old_fun_call->pos, old_fun_call->lang_type, false, def_name);
        log(LOG_DEBUG, LANG_TYPE_FMT"\n", lang_type_print(LANG_TYPE_MODE_LOG, old_fun_call->lang_type));
        try(sym_tbl_add(&vec_at(&env->ancesters, 0)->symbol_table, tast_variable_def_wrap(def)));
        
        vec_append(&a_main, &new_args, def_name);
        load_variable_def(env, new_block, def);
        fun_lang_type = lang_type_void_const_wrap(lang_type_void_new(0));
        //unreachable(TAST_FMT, tast_function_call_print(old_fun_call));
    }

    Llvm_function_call* new_fun_call = llvm_function_call_new(
        old_fun_call->pos,
        new_args,
        util_literal_name_new(),
        old_fun_call->name,
        0,
        fun_lang_type
    );
    try(alloca_add(env, llvm_expr_wrap(llvm_function_call_wrap(new_fun_call))));

    for (size_t idx = 0; idx < old_fun_call->args.info.count; idx++) {
        Tast_expr* old_arg = vec_at(&old_fun_call->args, idx);
        Str_view thing = load_expr(env, new_block, old_arg);
        vec_append(&a_main, &new_fun_call->args, thing);
        Llvm* result = NULL;
        try(alloca_lookup(&result, env, thing));
        log(LOG_DEBUG, TAST_FMT, llvm_print(result));
        log(LOG_DEBUG, TAST_FMT, llvm_function_call_print(new_fun_call));
    }

    vec_append(&a_main, &new_block->children, llvm_expr_wrap(llvm_function_call_wrap(new_fun_call)));

    if (rtn_is_struct) {
        assert(def_name.count > 0);

        Tast_symbol* new_sym = tast_symbol_new(old_fun_call->pos, (Sym_typed_base) {
           .name = def_name,
           .lang_type = old_fun_call->lang_type,
           .llvm_id = 0
        });

        Str_view result = load_expr(env, new_block, tast_symbol_wrap(new_sym));
        return result;
    } else {
        return new_fun_call->name_self;
    }
}

static Llvm_literal* tast_literal_clone(Tast_literal* old_lit) {
    switch (old_lit->type) {
        case TAST_STRING: {
            Llvm_string* string = llvm_string_new(
                tast_literal_get_pos(old_lit),
                tast_string_unwrap(old_lit)->data,
                tast_string_unwrap(old_lit)->name
            );
            return llvm_string_wrap(string);
        }
        case TAST_VOID: {
            return llvm_void_wrap(llvm_void_new(tast_literal_get_pos(old_lit), util_literal_name_new()));
        }
        case TAST_ENUM_LIT: {
            Llvm_enum_lit* enum_lit = llvm_enum_lit_new(
                tast_literal_get_pos(old_lit),
                tast_enum_lit_unwrap(old_lit)->data,
                tast_enum_lit_unwrap(old_lit)->lang_type,
                util_literal_name_new()
            );
            return llvm_enum_lit_wrap(enum_lit);
        }
        case TAST_CHAR: {
            Llvm_char* lang_char = llvm_char_new(
                tast_literal_get_pos(old_lit),
                tast_char_unwrap(old_lit)->data,
                util_literal_name_new()
            );
            return llvm_char_wrap(lang_char);
        }
        case TAST_NUMBER: {
            Llvm_number* number = llvm_number_new(
                tast_literal_get_pos(old_lit),
                tast_number_unwrap(old_lit)->data,
                tast_number_unwrap(old_lit)->lang_type,
                util_literal_name_new()
            );
            return llvm_number_wrap(number);
        }
        case TAST_SUM_LIT: {
            unreachable("sum literal should not be here");
        }
        case TAST_RAW_UNION_LIT: {
            unreachable("union literal should not be here");
        }
    }
    unreachable("");
}

static Str_view load_literal(
    Env* env,
    Llvm_block* new_block,
    Tast_literal* old_lit
) {
    (void) env;
    (void) new_block;
    Pos pos = tast_literal_get_pos(old_lit);

    Llvm_literal* new_lit = tast_literal_clone(old_lit);

    if (new_lit->type == LLVM_STRING) {
        Tast_string_def* new_def = tast_string_def_new(
            pos,
            tast_string_unwrap(old_lit)->name,
            tast_string_unwrap(old_lit)->data
        );
        log(LOG_DEBUG, TAST_FMT, tast_string_def_print(new_def));
        try(sym_tbl_add(&env->global_literals, tast_literal_def_wrap(tast_string_def_wrap(new_def))));
    }

    try(alloca_add(env, llvm_expr_wrap(llvm_literal_wrap(new_lit))));

    return llvm_literal_get_name(new_lit);
}

static Str_view load_ptr_symbol(
    Env* env,
    Llvm_block* new_block,
    Tast_symbol* old_sym
) {
    (void) new_block;

    Tast_def* var_def_ = NULL;
    log(LOG_DEBUG, TAST_FMT, tast_symbol_print(old_sym));
    try(symbol_lookup(&var_def_, env, old_sym->base.name));
    Llvm_variable_def* var_def = load_variable_def_clone(tast_variable_def_unwrap(var_def_));
    Llvm* alloca = NULL;
    if (!alloca_lookup(&alloca, env, var_def->name_corr_param)) {
        log(LOG_DEBUG, STR_VIEW_FMT"\n", str_view_print(var_def->name_corr_param));
        unreachable("");
    }

    assert(var_def);
    log(LOG_DEBUG, TAST_FMT, llvm_variable_def_print(var_def));
    log(LOG_DEBUG, TAST_FMT, tast_symbol_print(old_sym));
    assert(lang_type_is_equal(var_def->lang_type, old_sym->base.lang_type));

    return llvm_tast_get_name(alloca);
}

static Str_view load_ptr_sum_callee(
    Env* env,
    Llvm_block* new_block,
    Tast_sum_callee* old_callee
) {
    (void) new_block;
    (void) env;
    (void) old_callee;
    log(LOG_DEBUG, "entering thing: "TAST_FMT"\n", tast_sum_callee_print(old_callee));

    //Tast_def* var_def_ = NULL;
    //try(symbol_lookup(&var_def_, env, old_callee->name));
    //return load_ptr_member_access(env, new_block, tast_member_access_new(
    //    old_callee->pos,
    //    old_callee->lang_type,
    //    old_callee->
    //));
    todo();

    //return llvm_tast_get_name(alloca);
}

static Str_view load_symbol(
    Env* env,
    Llvm_block* new_block,
    Tast_symbol* old_sym
) {
    Pos pos = tast_symbol_get_pos(old_sym);

    Llvm_load_another_llvm* new_load = llvm_load_another_llvm_new(
        pos,
        load_ptr_symbol(env, new_block, old_sym),
        0,
        old_sym->base.lang_type,
        util_literal_name_new()
    );
    try(alloca_add(env, llvm_load_another_llvm_wrap(new_load)));

    vec_append(&a_main, &new_block->children, llvm_load_another_llvm_wrap(new_load));
    return new_load->name;
}

static Str_view load_binary_short_circuit(
    Env* env,
    Llvm_block* new_block,
    Tast_binary* old_bin
) {
    Lang_type u1_lang_type = lang_type_primitive_const_wrap(lang_type_unsigned_int_const_wrap(
        lang_type_unsigned_int_new(1, 0)
    ));

    Tast_variable_def* new_var_def = tast_variable_def_new(
        old_bin->pos,
        u1_lang_type,
        false,
        util_literal_name_new_prefix("short_cir")
    );
    try(sym_tbl_add(&vec_at(&env->ancesters, 0)->symbol_table, tast_variable_def_wrap(new_var_def)));

    Tast_stmt_vec if_true_stmts = {0};
    vec_append(&a_main, &if_true_stmts, tast_assignment_wrap(tast_assignment_new(
        old_bin->pos,
        tast_expr_wrap(tast_symbol_wrap(tast_symbol_new(old_bin->pos, (Sym_typed_base) {
            .lang_type = u1_lang_type,
            .name = new_var_def->name,
            .llvm_id = 0
        }))),
        tast_literal_wrap(
            util_tast_literal_new_from_int64_t(1, TOKEN_INT_LITERAL, old_bin->pos)
        )
    )));

    Tast_stmt_vec if_false_stmts = {0};
    vec_append(&a_main, &if_false_stmts, tast_assignment_wrap(tast_assignment_new(
        old_bin->pos,
        tast_expr_wrap(tast_symbol_wrap(tast_symbol_new(old_bin->pos, (Sym_typed_base) {
            .lang_type = u1_lang_type,
            .name = new_var_def->name,
            .llvm_id = 0
        }))),
        tast_literal_wrap(
            util_tast_literal_new_from_int64_t(0, TOKEN_INT_LITERAL, old_bin->pos)
        )
    )));

    switch (old_bin->token_type) {
        case BINARY_LOGICAL_AND: {
            Tast_if* if_true = tast_if_new(
                old_bin->pos,
                tast_condition_new(old_bin->pos, tast_binary_wrap(tast_binary_new(
                    old_bin->pos,
                    tast_operator_wrap(tast_unary_wrap(tast_unary_new(
                        old_bin->pos, old_bin->lhs, UNARY_UNSAFE_CAST, u1_lang_type
                    ))),
                    tast_operator_wrap(tast_unary_wrap(tast_unary_new(
                        old_bin->pos, old_bin->rhs, UNARY_UNSAFE_CAST, u1_lang_type
                    ))),
                    BINARY_BITWISE_AND,
                    u1_lang_type
                ))),
                // TODO: load inner expr in block, and update new symbol
                tast_block_new(old_bin->pos, if_true_stmts, (Symbol_collection) {0}, old_bin->pos)
            );

            Tast_if* if_false = tast_if_new(
                old_bin->pos,
                tast_condition_new(old_bin->pos, tast_binary_wrap(tast_binary_new(
                    old_bin->pos,
                    tast_literal_wrap(
                        util_tast_literal_new_from_int64_t(0, TOKEN_INT_LITERAL, old_bin->pos)
                    ),
                    tast_literal_wrap(
                        util_tast_literal_new_from_int64_t(0, TOKEN_INT_LITERAL, old_bin->pos)
                    ),
                    BINARY_DOUBLE_EQUAL,
                    u1_lang_type
                ))),
                // TODO: load inner expr in block, and update new symbol
                tast_block_new(old_bin->pos, if_false_stmts, (Symbol_collection) {0}, old_bin->pos)
            );

            Tast_if_vec ifs = {0};
            vec_append(&a_main, &ifs, if_true);
            vec_append(&a_main, &ifs, if_false);
            Tast_if_else_chain* if_else = tast_if_else_chain_new(old_bin->pos, ifs);
            
            //log(LOG_DEBUG, TAST_FMT, tast_if_print(if_true));
            //log(LOG_DEBUG, TAST_FMT, tast_if_print(if_false));
            log(LOG_DEBUG, TAST_FMT, tast_if_else_chain_print(if_else));
            load_variable_def(env, new_block, new_var_def);
            load_if_else_chain(env, new_block, if_else);
            return load_symbol(env, new_block, tast_symbol_new(old_bin->pos, (Sym_typed_base) {
                .lang_type = u1_lang_type,
                .name = new_var_def->name,
                .llvm_id = 0
            }));
        }
        case BINARY_LOGICAL_OR:
            todo();
        default:
            unreachable("");
    }
}

//static Str_view load_binary_short_circuit(
//    Env* env,
//    Llvm_block* new_block,
//    Tast_binary* old_bin
//) {
//    Lang_type u1_lang_type = lang_type_primitive_const_wrap(lang_type_unsigned_int_const_wrap(
//        lang_type_unsigned_int_new(1, 0)
//    ));
//
//    switch (old_bin->token_type) {
//        case BINARY_LOGICAL_AND: {
//
//        }
//        case BINARY_LOGICAL_OR:
//            todo();
//        default:
//            unreachable("");
//    }
//}

static Str_view load_binary(
    Env* env,
    Llvm_block* new_block,
    Tast_binary* old_bin
) {
    if (binary_is_short_circuit(old_bin->token_type)) {
        return load_binary_short_circuit(env, new_block, old_bin);
    }

    Llvm_binary* new_bin = llvm_binary_new(
        old_bin->pos,
        load_expr(env, new_block, old_bin->lhs),
        load_expr(env, new_block, old_bin->rhs),
        old_bin->token_type,
        old_bin->lang_type,
        0,
        util_literal_name_new()
    );

    try(alloca_add(env, llvm_expr_wrap(llvm_operator_wrap(llvm_binary_wrap(new_bin)))));

    vec_append(&a_main, &new_block->children, llvm_expr_wrap(llvm_operator_wrap(llvm_binary_wrap(new_bin))));
    return new_bin->name;
}

static Str_view load_deref(
    Env* env,
    Llvm_block* new_block,
    Tast_unary* old_unary
) {
    assert(old_unary->token_type == UNARY_DEREF);

    switch (old_unary->lang_type.type) {
        case LANG_TYPE_STRUCT:
            todo();
        case LANG_TYPE_PRIMITIVE: {
            Str_view ptr = load_expr(env, new_block, old_unary->child);
            Llvm_load_another_llvm* new_load = llvm_load_another_llvm_new(
                old_unary->pos,
                ptr,
                0,
                old_unary->lang_type,
                util_literal_name_new()
            );
            try(alloca_add(env, llvm_load_another_llvm_wrap(new_load)));

            vec_append(&a_main, &new_block->children, llvm_load_another_llvm_wrap(new_load));
            return new_load->name;
        }
        default:
            todo();
    }
    unreachable("");
}

static Str_view load_unary(
    Env* env,
    Llvm_block* new_block,
    Tast_unary* old_unary
) {
    switch (old_unary->token_type) {
        case UNARY_DEREF:
            return load_deref(env, new_block, old_unary);
        case UNARY_REFER:
            return load_ptr_expr(env, new_block, old_unary->child);
        case UNARY_UNSAFE_CAST:
            switch (old_unary->lang_type.type) {
                case LANG_TYPE_SUM:
                    return load_expr(env, new_block, old_unary->child);
                case LANG_TYPE_STRUCT:
                    return load_expr(env, new_block, old_unary->child);
                case LANG_TYPE_RAW_UNION:
                    return load_expr(env, new_block, old_unary->child);
                default:
                    break;
            }

            if (lang_type_get_pointer_depth(old_unary->lang_type) > 0 && lang_type_get_pointer_depth(tast_expr_get_lang_type(old_unary->child)) > 0) {
                return load_expr(env, new_block, old_unary->child);
            }

            // fallthrough
            Llvm_unary* new_unary = llvm_unary_new(
                old_unary->pos,
                load_expr(env, new_block, old_unary->child),
                old_unary->token_type,
                old_unary->lang_type,
                0,
                util_literal_name_new()
            );
            try(alloca_add(env, llvm_expr_wrap(llvm_operator_wrap(llvm_unary_wrap(new_unary)))));

            vec_append(&a_main, &new_block->children, llvm_expr_wrap(llvm_operator_wrap(llvm_unary_wrap(new_unary))));
            return new_unary->name;
        case UNARY_NOT:
            unreachable("not should not still be present here");
    }
    unreachable("");
}

static Str_view load_operator(
    Env* env,
    Llvm_block* new_block,
    Tast_operator* old_oper
) {
    switch (old_oper->type) {
        case TAST_BINARY:
            return load_binary(env, new_block, tast_binary_unwrap(old_oper));
        case TAST_UNARY:
            return load_unary(env, new_block, tast_unary_unwrap(old_oper));
    }
    unreachable("");
}

static Str_view load_ptr_member_access(
    Env* env,
    Llvm_block* new_block,
    Tast_member_access* old_access
) {
    Str_view new_callee = load_ptr_expr(env, new_block, old_access->callee);

    Tast_def* def = NULL;
    log(LOG_DEBUG, TAST_FMT, tast_member_access_print(old_access));
    log(LOG_DEBUG, LANG_TYPE_FMT"\n", str_view_print(lang_type_get_str(lang_type_from_get_name(env, new_callee))));
    try(symbol_lookup(&def, env, lang_type_get_str(lang_type_from_get_name(env, new_callee))));

    int64_t struct_index = {0};
    switch (def->type) {
        case TAST_STRUCT_DEF: {
            Tast_struct_def* struct_def = tast_struct_def_unwrap(def);
            struct_index = tast_get_member_index(&struct_def->base, old_access->member_name);
            break;
        }
        case TAST_RAW_UNION_DEF: {
            struct_index = 0;
            break;
        }
        default:
            unreachable("");
    }

    Tast_number* new_index = tast_number_new(
        old_access->pos,
        struct_index,
        lang_type_primitive_const_wrap(lang_type_signed_int_const_wrap(lang_type_signed_int_new(64, 0)))
    );
    
    Llvm_load_element_ptr* new_load = llvm_load_element_ptr_new(
        old_access->pos,
        old_access->lang_type,
        0,
        load_literal(env, new_block, tast_number_wrap(new_index)),
        new_callee,
        util_literal_name_new(),
        old_access->member_name,
        true
    );
    try(alloca_add(env, llvm_load_element_ptr_wrap(new_load)));

    vec_append(&a_main, &new_block->children, llvm_load_element_ptr_wrap(new_load));
    return new_load->name_self;
}

static Str_view load_ptr_index(
    Env* env,
    Llvm_block* new_block,
    Tast_index* old_index
) {
    Llvm_load_element_ptr* new_load = llvm_load_element_ptr_new(
        old_index->pos,
        old_index->lang_type,
        0,
        load_expr(env, new_block, old_index->index),
        load_expr(env, new_block, old_index->callee),
        util_literal_name_new(),
        str_view_from_cstr(""),
        false
    );
    try(alloca_add(env, llvm_load_element_ptr_wrap(new_load)));

    vec_append(&a_main, &new_block->children, llvm_load_element_ptr_wrap(new_load));
    return new_load->name_self;
}

static Str_view load_member_access(
    Env* env,
    Llvm_block* new_block,
    Tast_member_access* old_access
) {
    Str_view ptr = load_ptr_member_access(env, new_block, old_access);

    Llvm_load_another_llvm* new_load = llvm_load_another_llvm_new(
        old_access->pos,
        ptr,
        0,
        lang_type_from_get_name(env, ptr),
        util_literal_name_new()
    );
    try(alloca_add(env, llvm_load_another_llvm_wrap(new_load)));

    vec_append(&a_main, &new_block->children, llvm_load_another_llvm_wrap(new_load));
    return new_load->name;
}

static Str_view load_index(
    Env* env,
    Llvm_block* new_block,
    Tast_index* old_index
) {
    Str_view ptr = load_ptr_index(env, new_block, old_index);

    Llvm_load_another_llvm* new_load = llvm_load_another_llvm_new(
        old_index->pos,
        ptr,
        0,
        lang_type_from_get_name(env, ptr),
        util_literal_name_new()
    );
    try(alloca_add(env, llvm_load_another_llvm_wrap(new_load)));

    vec_append(&a_main, &new_block->children, llvm_load_another_llvm_wrap(new_load));
    return new_load->name;
}

static Str_view load_expr(Env* env, Llvm_block* new_block, Tast_expr* old_expr) {
    switch (old_expr->type) {
        case TAST_FUNCTION_CALL:
            return load_function_call(env, new_block, tast_function_call_unwrap(old_expr));
        case TAST_LITERAL:
            return load_literal(env, new_block, tast_literal_unwrap(old_expr));
        case TAST_SYMBOL:
            return load_symbol(env, new_block, tast_symbol_unwrap(old_expr));
        case TAST_OPERATOR:
            return load_operator(env, new_block, tast_operator_unwrap(old_expr));
        case TAST_MEMBER_ACCESS:
            return load_member_access(env, new_block, tast_member_access_unwrap(old_expr));
        case TAST_INDEX:
            return load_index(env, new_block, tast_index_unwrap(old_expr));
        case TAST_STRUCT_LITERAL:
            unreachable("struct literal should have been converted in remove_tuple pass");
        default:
            unreachable(TAST_FMT"\n", tast_expr_print(old_expr));
    }
}

static Llvm_reg load_function_parameters(
    Env* env,
    Llvm_block* new_fun_body,
    Llvm_lang_type** new_lang_type,
    Tast_lang_type* rtn_type,
    Tast_function_params* old_params
) {
    Llvm_function_params* new_params = do_function_def_alloca(env, new_lang_type, rtn_type, new_fun_body, old_params);

    for (size_t idx = 0; idx < new_params->params.info.count; idx++) {
        assert(env->ancesters.info.count > 0);
        Llvm_variable_def* param = vec_at(&new_params->params, idx);

        Llvm* dummy = NULL;
        //symbol_log(LOG_DEBUG, env);
        //alloca_log(LOG_DEBUG, env);

        bool is_struct = is_struct_like(param->lang_type.type);

        if (!is_struct) {
            log(LOG_DEBUG, TAST_FMT, llvm_variable_def_print(param));
            try(alloca_add(env, llvm_def_wrap(llvm_variable_def_wrap(param))));

            Llvm_store_another_llvm* new_store = llvm_store_another_llvm_new(
                param->pos,
                param->name_self,
                param->name_corr_param,
                0,
                param->lang_type,
                util_literal_name_new()
            );
            try(alloca_add(env, llvm_store_another_llvm_wrap(new_store)));

            vec_append(&a_main, &new_fun_body->children, llvm_store_another_llvm_wrap(new_store));
        }

        log(LOG_DEBUG, TAST_FMT"\n", llvm_variable_def_print(param));
        try(alloca_lookup(&dummy, env, param->name_corr_param));
    }

    return (Llvm_reg) {
        .lang_type = {0},
        .llvm = llvm_function_params_wrap(new_params)
    };
}

static Str_view load_function_def(
    Env* env,
    Tast_function_def* old_fun_def
) {
    Str_view old_fun_name = env->name_parent_function;
    env->name_parent_function = old_fun_def->decl->name;
    Pos pos = old_fun_def->pos;

    Llvm_function_decl* new_decl = llvm_function_decl_new(
        pos,
        NULL,
        load_lang_type_clone(old_fun_def->decl->return_type),
        old_fun_def->decl->name
    );

    Llvm_function_def* new_fun_def = llvm_function_def_new(
        pos,
        new_decl,
        llvm_block_new(
            pos,
            (Llvm_vec) {0},
            old_fun_def->body->symbol_collection,
            old_fun_def->body->pos_end
        ),
        0
    );

    //try(old_fun_def->decl->return_type->lang_type.info.count == 1);

    vec_append(&a_main, &env->ancesters, &new_fun_def->body->symbol_collection);
    Llvm_lang_type* new_lang_type = NULL;
    new_fun_def->decl->params = llvm_function_params_unwrap(load_function_parameters(
            env, new_fun_def->body, &new_lang_type, old_fun_def->decl->return_type, old_fun_def->decl->params
    ).llvm);
    assert(new_lang_type);
    new_fun_def->decl->return_type = new_lang_type;
    for (size_t idx = 0; idx < old_fun_def->body->children.info.count; idx++) {
        log(LOG_DEBUG, STR_VIEW_FMT, tast_stmt_print(vec_at(&old_fun_def->body->children, idx)));
        load_stmt(env, new_fun_def->body, vec_at(&old_fun_def->body->children, idx));
    }
    vec_rem_last(&env->ancesters);

    try(alloca_add(env, llvm_def_wrap(llvm_function_def_wrap(new_fun_def))));
    env->name_parent_function = old_fun_name;
    return (Str_view) {0};
}

static Str_view load_function_decl(
    Env* env,
    Tast_function_decl* old_fun_decl
) {
    try(alloca_add(env, llvm_def_wrap(llvm_function_decl_wrap(load_function_decl_clone(old_fun_decl)))));

    return (Str_view) {0};
}

static Str_view load_return(
    Env* env,
    Llvm_block* new_block,
    Tast_return* old_return
) {
    Pos pos = old_return->pos;

    Tast_def* fun_def_ = NULL;
    try(symbol_lookup(&fun_def_, env, env->name_parent_function));
    log(LOG_DEBUG, LLVM_FMT, tast_def_print(fun_def_));

    Tast_function_decl* fun_decl = NULL;
    switch (fun_def_->type) {
        case TAST_FUNCTION_DEF:
            fun_decl = tast_function_def_unwrap(fun_def_)->decl;
            break;
        case TAST_FUNCTION_DECL:
            fun_decl = tast_function_decl_unwrap(fun_def_);
            break;
        default:
            unreachable("");
    }

    //assert(fun_decl->return_type->lang_type.info.count == 1);
    Lang_type rtn_type = fun_decl->return_type->lang_type;

    bool rtn_is_struct = is_struct_like(rtn_type.type);

    if (rtn_is_struct) {
        Llvm* dest_ = NULL;
        try(alloca_lookup(&dest_, env, env->struct_rtn_name_parent_function));
        log(LOG_DEBUG, LLVM_FMT, llvm_print(dest_));
        Str_view dest = env->struct_rtn_name_parent_function;

        log(LOG_DEBUG, LLVM_FMT, tast_expr_print(old_return->child));
        Str_view src = load_expr(env, new_block, old_return->child);

        Llvm_store_another_llvm* new_store = llvm_store_another_llvm_new(
            pos,
            src,
            dest,
            0,
            rtn_type,
            util_literal_name_new()
        );
        vec_append(&a_main, &new_block->children, llvm_store_another_llvm_wrap(new_store));
        
        Tast_void* new_void = tast_void_new(old_return->pos);
        Llvm_return* new_return = llvm_return_new(
            pos,
            load_literal(env, new_block, tast_void_wrap(new_void)),
            old_return->is_auto_inserted
        );
        vec_append(&a_main, &new_block->children, llvm_return_wrap(new_return));
    } else {
        Str_view result = load_expr(env, new_block, old_return->child);
        Llvm_return* new_return = llvm_return_new(
            pos,
            result,
            old_return->is_auto_inserted
        );

        vec_append(&a_main, &new_block->children, llvm_return_wrap(new_return));
    }

    return (Str_view) {0};
}

static Str_view load_assignment(
    Env* env,
    Llvm_block* new_block,
    Tast_assignment* old_assignment
) {
    assert(old_assignment->lhs);
    assert(old_assignment->rhs);

    Pos pos = old_assignment->pos;

    Llvm_store_another_llvm* new_store = llvm_store_another_llvm_new(
        pos,
        load_expr(env, new_block, old_assignment->rhs),
        load_ptr_stmt(env, new_block, old_assignment->lhs),
        0,
        tast_stmt_get_lang_type(old_assignment->lhs),
        util_literal_name_new()
    );
    try(alloca_add(env, llvm_store_another_llvm_wrap(new_store)));

    assert(new_store->llvm_src.count > 0);
    assert(new_store->llvm_dest.count > 0);
    assert(old_assignment->rhs);

    vec_append(&a_main, &new_block->children, llvm_store_another_llvm_wrap(new_store));

    return new_store->name;
}

static Str_view load_variable_def(
    Env* env,
    Llvm_block* new_block,
    Tast_variable_def* old_var_def
) {
    Llvm_variable_def* new_var_def = load_variable_def_clone(old_var_def);

    Llvm* alloca = NULL;
    if (!alloca_lookup(&alloca, env, new_var_def->name_self)) {
        alloca = llvm_alloca_wrap(add_load_and_store_alloca_new(env, new_var_def));
        vec_insert(&a_main, &new_block->children, 0, alloca);
    }

    vec_append(&a_main, &new_block->children, llvm_def_wrap(llvm_variable_def_wrap(new_var_def)));

    assert(alloca);
    return llvm_tast_get_name(alloca);
}

static Str_view load_struct_def(
    Env* env,
    Tast_struct_def* old_struct_def
) {
    alloca_add(env, llvm_def_wrap(
        llvm_struct_def_wrap(load_struct_def_clone(old_struct_def))
    ));

    Tast_def* dummy = NULL;
    if (!symbol_lookup(&dummy, env, serialize_tast_struct_def(env, old_struct_def))) {
        Tast_struct_def* new_def = tast_struct_def_new(old_struct_def->pos, (Struct_def_base) {
            .members = old_struct_def->base.members, 
            .name = serialize_tast_struct_def(env, old_struct_def)
        });
        try(sym_tbl_add(&vec_at(&env->ancesters, 0)->symbol_table, tast_struct_def_wrap(new_def)));
        load_struct_def(env, new_def);
    }

    return (Str_view) {0};
}

static Str_view load_enum_def(
    Env* env,
    Tast_enum_def* old_enum_def
) {
    (void) env;

    try(alloca_add(env, llvm_def_wrap(
        llvm_enum_def_wrap(load_enum_def_clone(old_enum_def))
    )));

    return (Str_view) {0};
}

static Llvm_block* if_statement_to_branch(Env* env, Tast_if* if_statement, Str_view next_if, Str_view after_chain) {
    Tast_block* old_block = if_statement->body;
    Llvm_block* inner_block = load_block(env, old_block);
    Llvm_block* new_block = llvm_block_new(
        old_block->pos,
        (Llvm_vec) {0},
        inner_block->symbol_collection,
        inner_block->pos_end
    );


    vec_append(&a_main, &env->ancesters, &new_block->symbol_collection);

    Tast_condition* if_cond = if_statement->condition;

    Tast_operator* old_oper = if_cond->child;

    Str_view if_body = util_literal_name_new_prefix("start_if_body");

    if_for_add_cond_goto(env, old_oper, new_block, if_body, next_if);

    add_label(env, new_block, if_body, old_block->pos, false);

    {
        vec_rem_last(&env->ancesters);

        vec_extend(&a_main, &new_block->children, &inner_block->children);

        vec_append(&a_main, &env->ancesters, &new_block->symbol_collection);
    }


    Llvm_goto* jmp_to_after_chain = llvm_goto_new(
        old_block->pos,
        after_chain,
        0
    );
    vec_append(&a_main, &new_block->children, llvm_goto_wrap(jmp_to_after_chain));

    vec_rem_last(&env->ancesters);
    return new_block;
}

static Llvm_block* if_else_chain_to_branch(Env* env, Tast_if_else_chain* if_else) {
    Llvm_block* new_block = llvm_block_new(
        if_else->pos,
        (Llvm_vec) {0},
        (Symbol_collection) {0},
        (Pos) {0}
    );

    Str_view if_after = util_literal_name_new_prefix("if_after");
    
    Llvm* dummy = NULL;
    Tast_def* dummy_def = NULL;

    Str_view next_if = {0};
    for (size_t idx = 0; idx < if_else->tasts.info.count; idx++) {
        if (idx + 1 == if_else->tasts.info.count) {
            next_if = if_after;
        } else {
            next_if = util_literal_name_new_prefix("next_if");
        }

        Llvm_block* if_block = if_statement_to_branch(env, vec_at(&if_else->tasts, idx), next_if, if_after);
        vec_append(&a_main, &new_block->children, llvm_block_wrap(if_block));

        if (idx + 1 < if_else->tasts.info.count) {
            assert(!alloca_lookup(&dummy, env, next_if));
            add_label(env, new_block, next_if, vec_at(&if_else->tasts, idx)->pos, false);
            assert(alloca_lookup(&dummy, env, next_if));
        } else {
            assert(str_view_is_equal(next_if, if_after));
        }
    }

    assert(!symbol_lookup(&dummy_def, env, next_if));
    add_label(env, new_block, if_after, if_else->pos, false);
    assert(alloca_lookup(&dummy, env, next_if));
    //log_tree(LOG_DEBUG, tast_block_wrap(new_block));

    return new_block;
}

static Str_view load_if_else_chain(
    Env* env,
    Llvm_block* new_block,
    Tast_if_else_chain* old_if_else
) {
    Llvm_block* new_if_else = if_else_chain_to_branch(env, old_if_else);
    vec_append(&a_main, &new_block->children, llvm_block_wrap(new_if_else));

    return (Str_view) {0};
}

static Llvm_block* for_range_to_branch(Env* env, Tast_for_range* old_for) {
    Str_view old_if_break = env->label_if_break;
    Str_view old_if_continue = env->label_if_continue;

    size_t init_count_ancesters = env->ancesters.info.count;

    Pos pos = old_for->pos;

    Llvm_block* new_branch_block = llvm_block_new(
        pos,
        (Llvm_vec) {0},
        old_for->body->symbol_collection,
        old_for->body->pos_end
    );

    for (size_t idx = 0; idx < old_for->body->children.info.count; idx++) {
        log(LOG_DEBUG, TAST_FMT"\n", tast_stmt_print(vec_at(&old_for->body->children, idx)));
    }
    for (size_t idx = 0; idx < new_branch_block->children.info.count; idx++) {
        log(LOG_DEBUG, TAST_FMT"\n", llvm_print(vec_at(&new_branch_block->children, idx)));
    }

    vec_append(&a_main, &env->ancesters, &new_branch_block->symbol_collection);

    (void) pos;

    //vec_append(&a_main, &env->ancesters, tast_block_wrap(for_block));
    Tast_expr* lhs_actual = old_for->lower_bound->child;
    Tast_expr* rhs_actual = old_for->upper_bound->child;

    Uast_symbol* symbol_lhs_assign_;
    Tast_symbol* symbol_lhs_assign;
    Tast_variable_def* for_var_def;
    {
        for_var_def = old_for->var_def;
        symbol_lhs_assign_ = uast_symbol_new(for_var_def->pos, for_var_def->name);
        Tast_expr* new_expr = NULL;
        try(try_set_symbol_type(env, &new_expr, symbol_lhs_assign_));
        log(LOG_DEBUG, STR_VIEW_FMT, tast_expr_print(new_expr));
        symbol_lhs_assign = tast_symbol_unwrap(new_expr);
    }

    //try(symbol_add(env, tast_variable_def_wrap(for_var_def)));
    Llvm* dummy = NULL;
    Tast_def* dummy_def = NULL;

    assert(symbol_lookup(&dummy_def, env, for_var_def->name));

    Tast_assignment* assignment_to_inc_cond_var = for_loop_cond_var_assign_new(
        env, for_var_def->name, tast_expr_get_pos(lhs_actual)
    );

    Uast_symbol* lhs_untyped = uast_symbol_new(tast_symbol_get_pos(symbol_lhs_assign), symbol_lhs_assign->base.name);
    Tast_expr* lhs_typed_ = NULL;
    try(try_set_symbol_type(env, &lhs_typed_, lhs_untyped));
    Tast_expr* operator_ = NULL;
    try(try_set_binary_types_finish(env, &operator_, lhs_typed_, rhs_actual, old_for->pos, BINARY_LESS_THAN)); 
    Tast_operator* operator = tast_operator_unwrap(operator_);
    //Tast_operator* operator = util_binary_typed_new(
    //    env,
    //    lhs_untyped,
    //    rhs_actual,
    //    TOKEN_LESS_THAN
    //);

    // initial assignment

    Str_view check_cond_label = util_literal_name_new_prefix("check_cond");
    Str_view assign_label = util_literal_name_new_prefix("assign_for");
    Llvm_goto* jmp_to_check_cond_label = llvm_goto_new(old_for->pos, check_cond_label, 0);
    Llvm_goto* jmp_to_assign = llvm_goto_new(old_for->pos, assign_label, 0);
    Str_view after_check_label = util_literal_name_new_prefix("after_check");
    Str_view after_for_loop_label = util_literal_name_new_prefix("after_for");

    env->label_if_break = after_for_loop_label;
    env->label_if_continue = assign_label;

    //vec_append(&a_main, &new_branch_block->children, tast_variable_def_wrap(for_var_def));

    Tast_assignment* new_var_assign = tast_assignment_new(tast_symbol_get_pos(symbol_lhs_assign), tast_expr_wrap(tast_symbol_wrap(symbol_lhs_assign)), lhs_actual);
    //Tast_assignment* new_var_assign = util_assignment_new(env, uast_expr_wrap(uast_symbol_wrap(symbol_lhs_assign)), lhs_actual);

    load_variable_def(env, new_branch_block, for_var_def);
    load_assignment(env, new_branch_block, new_var_assign);
    
    //log_tree(LOG_DEBUG, (Tast*)new_var_assign);

    vec_append(&a_main, &new_branch_block->children, llvm_goto_wrap(jmp_to_check_cond_label));

    assert(!alloca_lookup(&dummy, env, check_cond_label));

    {
        vec_rem_last(&env->ancesters);
        add_label(env, new_branch_block, check_cond_label, pos, false);
        vec_append(&a_main, &env->ancesters, &new_branch_block->symbol_collection);
    }

    assert(alloca_lookup(&dummy, env, check_cond_label));

    load_operator(env, new_branch_block, operator);
    assert(alloca_lookup(&dummy, env, check_cond_label));

    if_for_add_cond_goto(
        env,
        operator,
        new_branch_block,
        after_check_label,
        after_for_loop_label
    );
    assert(alloca_lookup(&dummy, env, check_cond_label));
    {
        vec_rem_last(&env->ancesters);
        add_label(env, new_branch_block, after_check_label, pos, false);
        vec_append(&a_main, &env->ancesters, &new_branch_block->symbol_collection);
    }

    assert(alloca_lookup(&dummy, env, check_cond_label));
    {

        for (size_t idx = 0; idx < old_for->body->children.info.count; idx++) {
            for (size_t idx = 0; idx < new_branch_block->children.info.count; idx++) {
                log(LOG_DEBUG, TAST_FMT"\n", llvm_print(vec_at(&new_branch_block->children, idx)));
            }
            load_stmt(env, new_branch_block, vec_at(&old_for->body->children, idx));
        }
    }
    assert(alloca_lookup(&dummy, env, check_cond_label));

    vec_append(&a_main, &new_branch_block->children, llvm_goto_wrap(jmp_to_assign));
    {
        vec_rem_last(&env->ancesters);
        add_label(env, new_branch_block, assign_label, pos, false);
        vec_append(&a_main, &env->ancesters, &new_branch_block->symbol_collection);
    }
    load_assignment(env, new_branch_block, assignment_to_inc_cond_var);
    vec_append(&a_main, &new_branch_block->children, llvm_goto_wrap(llvm_goto_new(pos, check_cond_label, 0)));
    add_label(env, new_branch_block, after_for_loop_label, pos, true);
    assert(alloca_lookup(&dummy, env, check_cond_label));

    try(alloca_do_add_defered(&dummy, env));

    assert(alloca_lookup(&dummy, env, check_cond_label));
    env->label_if_break = old_if_break;
    env->label_if_continue = old_if_continue;
    assert(alloca_lookup(&dummy, env, check_cond_label));
    vec_rem_last(&env->ancesters);

    assert(init_count_ancesters == env->ancesters.info.count);

    return new_branch_block;
}

static Str_view load_for_range(
    Env* env,
    Llvm_block* new_block,
    Tast_for_range* old_for
) {
    Llvm_block* new_for = for_range_to_branch(env, old_for);
    vec_append(&a_main, &new_block->children, llvm_block_wrap(new_for));

    return (Str_view) {0};
}

static Llvm_block* for_with_cond_to_branch(Env* env, Tast_for_with_cond* old_for) {
    Str_view old_if_break = env->label_if_break;
    Str_view old_if_continue = env->label_if_continue;

    Pos pos = old_for->pos;

    Llvm_block* new_branch_block = llvm_block_new(
        pos,
        (Llvm_vec) {0},
        old_for->body->symbol_collection,
        old_for->body->pos_end
    );

    vec_append(&a_main, &env->ancesters, &new_branch_block->symbol_collection);


    Tast_operator* operator = old_for->condition->child;
    Str_view check_cond_label = util_literal_name_new_prefix("check_cond");
    Llvm_goto* jmp_to_check_cond_label = llvm_goto_new(old_for->pos, check_cond_label, 0);
    Str_view after_check_label = util_literal_name_new_prefix("for_body");
    Str_view after_for_loop_label = util_literal_name_new_prefix("after_for_loop");

    env->label_if_break = after_for_loop_label;
    env->label_if_continue = check_cond_label;

    vec_append(&a_main, &new_branch_block->children, llvm_goto_wrap(jmp_to_check_cond_label));

    add_label(env, new_branch_block, check_cond_label, pos, true);

    //load_operator(env, new_branch_block, operator);
    //Tast* dummy = NULL;
    //try(symbol_lookup(&dummy, env, str_view_from_cstr("str18")));

    if_for_add_cond_goto(
        env,
        operator,
        new_branch_block,
        after_check_label,
        after_for_loop_label
    );

    add_label(env, new_branch_block, after_check_label, pos, true);

    //for (size_t idx = 0; idx < new_branch_block->children.info.count; idx++) {
    //    log(LOG_DEBUG, TAST_FMT"\n", tast_print(vec_at(&new_branch_block->children, idx)));
    //}
    log(LOG_DEBUG, "DSFJKLKJDFS: "STR_VIEW_FMT"\n", str_view_print(after_check_label));


    //try(symbol_lookup(&dummy, env, str_view_from_cstr("str18")));
    //for (size_t idx = 0; idx < old_for->body->children.info.count; idx++) {
    //    log(LOG_DEBUG, TAST_FMT"\n", tast_print(vec_at(&old_for->body->children, idx)));
    //}

    for (size_t idx = 0; idx < old_for->body->children.info.count; idx++) {
        //for (size_t idx = 0; idx < new_branch_block->children.info.count; idx++) {
        //    log(LOG_DEBUG, TAST_FMT"\n", tast_print(vec_at(&new_branch_block->children, idx)));
        //}
        load_stmt(env, new_branch_block, vec_at(&old_for->body->children, idx));
    }

    vec_append(&a_main, &new_branch_block->children, llvm_goto_wrap(
        llvm_goto_new(old_for->pos, check_cond_label, 0)
    ));
    add_label(env, new_branch_block, after_for_loop_label, pos, true);

    Llvm* dummy = NULL;

    env->label_if_continue = old_if_continue;
    env->label_if_break = old_if_break;
    for (size_t idx = 0; idx < env->defered_allocas_to_add.info.count; idx++) {
        log(LOG_DEBUG, TAST_FMT, llvm_print(vec_at(&env->defered_allocas_to_add, idx)));
    }
    log_env(LOG_DEBUG, env);
    vec_rem_last(&env->ancesters);
    Symbol_collection* popped = NULL;
    vec_pop(popped, &env->ancesters);
    try(alloca_do_add_defered(&dummy, env));
    vec_append(&a_main, &env->ancesters, popped);

    return new_branch_block;
}

static Str_view load_for_with_cond(
    Env* env,
    Llvm_block* new_block,
    Tast_for_with_cond* old_for
) {
    Llvm_block* new_for = for_with_cond_to_branch(env, old_for);
    vec_append(&a_main, &new_block->children, llvm_block_wrap(new_for));

    return (Str_view) {0};
}

static Str_view load_break(
    Env* env,
    Llvm_block* new_block,
    Tast_break* old_break
) {
    if (env->label_if_break.count < 1) {
        msg(
            LOG_ERROR, EXPECT_FAIL_BREAK_INVALID_LOCATION, env->file_text, old_break->pos,
            "break statement outside of a for loop\n"
        );
        return (Str_view) {0};
    }

    Llvm_goto* new_goto = llvm_goto_new(old_break->pos, env->label_if_break, 0);
    vec_append(&a_main, &new_block->children, llvm_goto_wrap(new_goto));

    return (Str_view) {0};
}

static Str_view load_continue(
    Env* env,
    Llvm_block* new_block,
    Tast_continue* old_continue
) {
    if (env->label_if_continue.count < 1) {
        msg(
            LOG_ERROR, EXPECT_FAIL_CONTINUE_INVALID_LOCATION, env->file_text, old_continue->pos,
            "continue statement outside of a for loop\n"
        );
        return (Str_view) {0};
    }

    Llvm_goto* new_goto = llvm_goto_new(old_continue->pos, env->label_if_continue, 0);
    vec_append(&a_main, &new_block->children, llvm_goto_wrap(new_goto));

    return (Str_view) {0};
}

static Str_view load_raw_union_def(
    Env* env,
    Tast_raw_union_def* old_def
) {
    log(LOG_DEBUG, TAST_FMT, tast_raw_union_def_print(old_def));
    // TODO: crash if alloca_add fails (we need to prevent duplicates to crash on alloca_add fail)?
    if (!alloca_add(env, llvm_def_wrap(
        llvm_raw_union_def_wrap(load_raw_union_def_clone(old_def))
    ))) {
        return (Str_view) {0};
    };

    Tast_def* dummy = NULL;
    if (!symbol_lookup(&dummy, env, serialize_tast_raw_union_def(env, old_def))) {
        Tast_raw_union_def* new_def = tast_raw_union_def_new(old_def->pos, (Struct_def_base) {
            .members = old_def->base.members, 
            .name = serialize_tast_raw_union_def(env, old_def)
        });
        try(sym_tbl_add(&vec_at(&env->ancesters, 0)->symbol_table, tast_raw_union_def_wrap(new_def)));
        load_raw_union_def(env, new_def);
    }

    return (Str_view) {0};
}

static Str_view load_ptr_variable_def(
    Env* env,
    Llvm_block* new_block,
    Tast_variable_def* old_variable_def
) {
    return load_variable_def(env, new_block, old_variable_def);
}

static Str_view load_ptr_deref(
    Env* env,
    Llvm_block* new_block,
    Tast_unary* old_unary
) {
    assert(old_unary->token_type == UNARY_DEREF);

    switch (old_unary->lang_type.type) {
        case LANG_TYPE_STRUCT:
            return load_ptr_expr(env, new_block, old_unary->child);
        case LANG_TYPE_PRIMITIVE:
            break;
        case LANG_TYPE_RAW_UNION:
            return load_ptr_expr(env, new_block, old_unary->child);
        case LANG_TYPE_ENUM:
            break;
        default:
            todo();
    }

    Str_view ptr = load_ptr_expr(env, new_block, old_unary->child);
    Llvm_load_another_llvm* new_load = llvm_load_another_llvm_new(
        old_unary->pos,
        ptr,
        0,
        old_unary->lang_type,
        util_literal_name_new()
    );
    try(alloca_add(env, llvm_load_another_llvm_wrap(new_load)));
    lang_type_set_pointer_depth(&new_load->lang_type, lang_type_get_pointer_depth(new_load->lang_type) + 1);

    vec_append(&a_main, &new_block->children, llvm_load_another_llvm_wrap(new_load));
    return new_load->name;
}

static Str_view load_ptr_unary(
    Env* env,
    Llvm_block* new_block,
    Tast_unary* old_unary
) {
    switch (old_unary->token_type) {
        case UNARY_DEREF:
            return load_ptr_deref(env, new_block, old_unary);
        default:
            todo();
    }
    todo();
}

static Str_view load_ptr_operator(Env* env, Llvm_block* new_block, Tast_operator* old_oper) {
    switch (old_oper->type) {
        case TAST_BINARY:
            todo();
        case TAST_UNARY:
            return load_ptr_unary(env, new_block, tast_unary_unwrap(old_oper));
        default:
            unreachable("");
    }
}

static Str_view load_ptr_expr(Env* env, Llvm_block* new_block, Tast_expr* old_expr) {
    switch (old_expr->type) {
        case TAST_SYMBOL:
            return load_ptr_symbol(env, new_block, tast_symbol_unwrap(old_expr));
        case TAST_MEMBER_ACCESS:
            return load_ptr_member_access(env, new_block, tast_member_access_unwrap(old_expr));
        case TAST_OPERATOR:
            return load_ptr_operator(env, new_block, tast_operator_unwrap(old_expr));
        case TAST_INDEX:
            return load_ptr_index(env, new_block, tast_index_unwrap(old_expr));
        case TAST_LITERAL:
            // TODO: deref string literal
            unreachable("");
        case TAST_FUNCTION_CALL:
            unreachable("");
        case TAST_STRUCT_LITERAL:
            unreachable("");
        case TAST_TUPLE:
            unreachable("");
        case TAST_SUM_CALLEE:
            return load_ptr_sum_callee(env, new_block, tast_sum_callee_unwrap(old_expr));
        case TAST_SUM_CASE:
            unreachable("");
        case TAST_SUM_ACCESS:
            unreachable("");
    }
    unreachable("");
}

static Str_view load_ptr_def(Env* env, Llvm_block* new_block, Tast_def* old_def) {
    switch (old_def->type) {
        case TAST_VARIABLE_DEF:
            return load_ptr_variable_def(env, new_block, tast_variable_def_unwrap(old_def));
        default:
            unreachable("");
    }
}

static Str_view load_ptr_stmt(Env* env, Llvm_block* new_block, Tast_stmt* old_stmt) {
    switch (old_stmt->type) {
        case TAST_EXPR:
            return load_ptr_expr(env, new_block, tast_expr_unwrap(old_stmt));
        case TAST_DEF:
            return load_ptr_def(env, new_block, tast_def_unwrap(old_stmt));
        default:
            unreachable("");
    }
}

static Str_view load_ptr(Env* env, Llvm_block* new_block, Tast* old_tast) {
    switch (old_tast->type) {
        case TAST_STMT:
            return load_ptr_stmt(env, new_block, tast_stmt_unwrap(old_tast));
        default:
            unreachable(TAST_FMT"\n", tast_print(old_tast));
    }
}

static Str_view load_def(Env* env, Llvm_block* new_block, Tast_def* old_def) {
    switch (old_def->type) {
        case TAST_FUNCTION_DEF:
            return load_function_def(env, tast_function_def_unwrap(old_def));
        case TAST_FUNCTION_DECL:
            return load_function_decl(env, tast_function_decl_unwrap(old_def));
        case TAST_VARIABLE_DEF:
            return load_variable_def(env, new_block, tast_variable_def_unwrap(old_def));
        case TAST_STRUCT_DEF:
            return load_struct_def(env, tast_struct_def_unwrap(old_def));
        case TAST_ENUM_DEF:
            return load_enum_def(env, tast_enum_def_unwrap(old_def));
        case TAST_RAW_UNION_DEF:
            return load_raw_union_def(env, tast_raw_union_def_unwrap(old_def));
        case TAST_SUM_DEF:
            unreachable("sum def should not make it here");
        case TAST_LITERAL_DEF:
            unreachable("");
        case TAST_PRIMITIVE_DEF:
            unreachable("");
    }
    unreachable("");
}

static Str_view load_stmt(Env* env, Llvm_block* new_block, Tast_stmt* old_stmt) {
    switch (old_stmt->type) {
        case TAST_EXPR:
            return load_expr(env, new_block, tast_expr_unwrap(old_stmt));
        case TAST_DEF:
            return load_def(env, new_block, tast_def_unwrap(old_stmt));
        case TAST_RETURN:
            return load_return(env, new_block, tast_return_unwrap(old_stmt));
        case TAST_ASSIGNMENT:
            return load_assignment(env, new_block, tast_assignment_unwrap(old_stmt));
        case TAST_IF_ELSE_CHAIN:
            return load_if_else_chain(env, new_block, tast_if_else_chain_unwrap(old_stmt));
        case TAST_FOR_RANGE:
            return load_for_range(env, new_block, tast_for_range_unwrap(old_stmt));
        case TAST_FOR_WITH_COND:
            return load_for_with_cond(env, new_block, tast_for_with_cond_unwrap(old_stmt));
        case TAST_BREAK:
            return load_break(env, new_block, tast_break_unwrap(old_stmt));
        case TAST_CONTINUE:
            return load_continue(env, new_block, tast_continue_unwrap(old_stmt));
        case TAST_BLOCK:
            vec_append(
                &a_main,
                &new_block->children,
                llvm_block_wrap(load_block(env, tast_block_unwrap(old_stmt)))
            );
            return (Str_view) {0};
        default:
            unreachable("");
    }
}

static Str_view load_tast(Env* env, Llvm_block* new_block, Tast* old_tast) {
    (void) env;
    (void) new_block;
    (void) old_tast;
    switch (old_tast->type) {
        default:
            unreachable(TAST_FMT"\n", tast_print(old_tast));
    }
}

static Str_view load_def_sometimes(Env* env, Tast_def* old_def) {
    switch (old_def->type) {
        case TAST_FUNCTION_DEF:
            return load_function_def(env, tast_function_def_unwrap(old_def));
        case TAST_FUNCTION_DECL:
            return load_function_decl(env, tast_function_decl_unwrap(old_def));
        case TAST_VARIABLE_DEF:
            return (Str_view) {0};
        case TAST_STRUCT_DEF:
            return load_struct_def(env, tast_struct_def_unwrap(old_def));
        case TAST_ENUM_DEF:
            return load_enum_def(env, tast_enum_def_unwrap(old_def));
        case TAST_RAW_UNION_DEF:
            return load_raw_union_def(env, tast_raw_union_def_unwrap(old_def));
        case TAST_SUM_DEF:
            return (Str_view) {0};
        case TAST_LITERAL_DEF:
            unreachable("");
        case TAST_PRIMITIVE_DEF:
            unreachable("");
    }
    unreachable("");
}

// TODO: rethink how to do block, because this is simply not working
static Llvm_block* load_block(Env* env, Tast_block* old_block) {
    size_t init_count_ancesters = env->ancesters.info.count;

    Llvm_block* new_block = llvm_block_new(
        old_block->pos,
        (Llvm_vec) {0},
        old_block->symbol_collection,
        old_block->pos_end
    );

    vec_append(&a_main, &env->ancesters, &new_block->symbol_collection);

    Symbol_table table = vec_top(&env->ancesters)->symbol_table;
    for (size_t idx = 0; idx < table.capacity; idx++) {
        if (table.table_tasts[idx].status != SYM_TBL_OCCUPIED) {
            continue;
        }

        load_def_sometimes(env, table.table_tasts[idx].tast);
    }

    for (size_t idx = 0; idx < old_block->children.info.count; idx++) {
        load_stmt(env, new_block, vec_at(&old_block->children, idx));
    }

    vec_rem_last(&env->ancesters);

    try(init_count_ancesters == env->ancesters.info.count);
    return new_block;
}

Llvm_block* add_load_and_store(Env* env, Tast_block* old_root) {
    Llvm_block* block = load_block(env, old_root);
    return block;
}

