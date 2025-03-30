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
#include <symbol_log.h>
#include <symbol_iter.h>

#include "passes.h"

// forward declarations

static Lang_type rm_tuple_lang_type(Env* env, Lang_type lang_type, Pos lang_type_pos);

static Str_view load_assignment(
    Env* env,
    Llvm_block* new_block,
    Tast_assignment* old_assignment
);

static Str_view load_symbol(
    Env* env,
    Llvm_block* new_block,
    Tast_symbol* old_symbol
);

static void load_struct_def(
    Env* env,
    Tast_struct_def* old_struct_def
);

static void load_raw_union_def(
    Env* env,
    Tast_raw_union_def* old_def
);

static Str_view load_ptr_symbol(
    Env* env,
    Llvm_block* new_block,
    Tast_symbol* old_sym
);

static Tast_symbol* tast_symbol_new_from_variable_def(Pos pos, const Tast_variable_def* def) {
    return tast_symbol_new(
        pos,
        (Sym_typed_base) {.lang_type = def->lang_type, .name = def->name, .llvm_id = 0}
    );
}

static Lang_type_struct rm_tuple_lang_type_tuple(Env* env, Lang_type_tuple lang_type, Pos lang_type_pos) {
    Tast_variable_def_vec members = {0};

    for (size_t idx = 0; idx < lang_type.lang_types.info.count; idx++) {
        Tast_variable_def* memb = tast_variable_def_new(
            lang_type_pos,
            rm_tuple_lang_type(env, vec_at_const(lang_type.lang_types, idx), lang_type_pos),
            false,
            util_literal_name_new_prefix("tuple_struct_member")
        );
        vec_append(&a_main, &members, memb);
    }

    Struct_def_base base = {
        .members = members,
        .name = serialize_lang_type_tuple(env, lang_type)
    };
    // todo: remove untyped things here
    Tast_struct_def* struct_def = tast_struct_def_new(lang_type_pos, base);
    // TODO: consider collisions with generated structs and user defined structs
    sym_tbl_add(&vec_at(&env->ancesters, 0)->symbol_table, tast_struct_def_wrap(struct_def));
    return lang_type_struct_new(lang_type_pos, lang_type_atom_new(base.name, 0));
}

// note: will not clone everything
static Tast_raw_union_def* get_raw_union_def_from_sum_def(Env* env, Tast_sum_def* sum_def) {
    // TODO: find way to avoid making new Tast_raw_union_def every time
    Tast_raw_union_def* union_def = tast_raw_union_def_new(sum_def->pos, sum_def->base);
    union_def->base.name = serialize_tast_raw_union_def(env, union_def);
    Tast_def* cached_def = NULL;
    if (symbol_lookup(&cached_def, env, union_def->base.name)) {
        return tast_raw_union_def_unwrap(cached_def);
    }
    sym_tbl_add(&vec_at(&env->ancesters, 0)->symbol_table, tast_raw_union_def_wrap(union_def));
    load_raw_union_def(env, union_def);
    return union_def;
}

static Lang_type rm_tuple_lang_type_sum(Env* env, Lang_type_sum lang_type, Pos lang_type_pos) {
    Tast_def* lang_type_def_ = NULL; 
    unwrap(symbol_lookup(&lang_type_def_, env, lang_type.atom.str));
    Tast_variable_def_vec members = {0};

    Tast_variable_def* tag = tast_variable_def_new(
        lang_type_pos,
        // TODO: make helper functions, etc. for line below, because this is too much to do every time
        lang_type_primitive_const_wrap(lang_type_signed_int_const_wrap(lang_type_signed_int_new(lang_type_pos, 64, 0))),
        false,
        str_view_from_cstr("tag")
    );
    vec_append(&a_main, &members, tag);

    Tast_raw_union_def* item_type_def = get_raw_union_def_from_sum_def(env, tast_sum_def_unwrap(lang_type_def_));

    for (size_t idx = 0; idx < item_type_def->base.members.info.count; idx++) {
        vec_at(&item_type_def->base.members, idx)->lang_type = rm_tuple_lang_type(
            env, 
            vec_at(&item_type_def->base.members, idx)->lang_type,
            item_type_def->pos
        );
    }

    Tast_variable_def* item = tast_variable_def_new(
        lang_type_pos,
        rm_tuple_lang_type(env, lang_type_raw_union_const_wrap(lang_type_raw_union_new(item_type_def->pos, lang_type_atom_new(item_type_def->base.name, 0))), lang_type_pos),
        false,
        str_view_from_cstr("item")
    );
    vec_append(&a_main, &members, item);

    Struct_def_base base = {
        .members = members,
        .name = util_literal_name_new_prefix("temp")
    };
    
    Tast_struct_def* struct_def = tast_struct_def_new(lang_type_pos, base);
    unwrap(sym_tbl_add(&vec_at(&env->ancesters, 0)->symbol_table, tast_struct_def_wrap(struct_def)));
    struct_def->base.name = serialize_tast_struct_def(env, struct_def);
    // TODO: consider collisions with generated structs and user defined structs
    sym_tbl_add(&vec_at(&env->ancesters, 0)->symbol_table, tast_struct_def_wrap(struct_def));
    Tast_def* dummy = NULL;
    unwrap(sym_tbl_lookup(&dummy, &vec_at(&env->ancesters, 0)->symbol_table, struct_def->base.name));

    load_struct_def(env, struct_def);
    return tast_struct_def_get_lang_type(struct_def);
}

static Lang_type rm_tuple_lang_type(Env* env, Lang_type lang_type, Pos lang_type_pos) {
    switch (lang_type.type) {
        case LANG_TYPE_SUM: {
            return rm_tuple_lang_type_sum(env, lang_type_sum_const_unwrap(lang_type), lang_type_pos);
        }
        case LANG_TYPE_RAW_UNION: {
            Tast_def* lang_type_def_ = NULL; 
            unwrap(symbol_lookup(&lang_type_def_, env, lang_type_get_str(lang_type)));
            Tast_variable_def_vec members = {0};

            Tast_variable_def* tag = tast_variable_def_new(
                lang_type_pos,
                // TODO: make helper functions, etc. for line below, because this is too much to do every time
                lang_type_primitive_const_wrap(lang_type_signed_int_const_wrap(lang_type_signed_int_new(POS_BUILTIN, 64, 0))),
                false,
                str_view_from_cstr("tag")
            );
            vec_append(&a_main, &members, tag);

            Tast_raw_union_def* item_type_def = tast_raw_union_def_new(
                lang_type_pos,
                tast_raw_union_def_unwrap(lang_type_def_)->base
            );
            item_type_def->base.name = serialize_tast_raw_union_def(env, item_type_def);
            sym_tbl_add(&vec_at(&env->ancesters, 0)->symbol_table, tast_raw_union_def_wrap(item_type_def));

            load_raw_union_def(env, item_type_def);
            return tast_raw_union_def_get_lang_type(item_type_def);
        }
        case LANG_TYPE_TUPLE:
            return lang_type_struct_const_wrap(rm_tuple_lang_type_tuple(env, lang_type_tuple_const_unwrap(lang_type), lang_type_pos));
        case LANG_TYPE_PRIMITIVE:
            return lang_type;
        case LANG_TYPE_STRUCT:
            return lang_type;
        case LANG_TYPE_VOID:
            return lang_type;
        case LANG_TYPE_ENUM:
            return lang_type;
        case LANG_TYPE_FN:
            return lang_type;
        default:
            unreachable(LANG_TYPE_FMT, lang_type_print(LANG_TYPE_MODE_LOG, lang_type));
    }
    unreachable("");
}

static bool binary_is_short_circuit(BINARY_TYPE type) {
    switch (type) {
        case BINARY_SINGLE_EQUAL:
            return false;
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
        case BINARY_SHIFT_LEFT:
            return false;
        case BINARY_SHIFT_RIGHT:
            return false;
    }
    unreachable("");
}

static Llvm_variable_def* load_variable_def_clone(Env* env, Tast_variable_def* old_var_def);

static Llvm_alloca* add_load_and_store_alloca_new(Env* env, Llvm_variable_def* var_def) {
    Llvm_alloca* alloca = llvm_alloca_new(var_def->pos, 0, rm_tuple_lang_type(env, var_def->lang_type, var_def->pos), var_def->name_corr_param);
    alloca_add(env, llvm_alloca_wrap(alloca));
    assert(alloca);
    return alloca;
}

static Llvm_function_params* load_function_params_clone(Env* env, Tast_function_params* old_params) {
    Llvm_function_params* new_params = llvm_function_params_new(old_params->pos, (Llvm_variable_def_vec){0}, 0);

    for (size_t idx = 0; idx < old_params->params.info.count; idx++) {
        vec_append(&a_main, &new_params->params, load_variable_def_clone(env, vec_at(&old_params->params, idx)));
    }

    return new_params;
}

static Llvm_function_decl* load_function_decl_clone(Env* env, Tast_function_decl* old_decl) {
    return llvm_function_decl_new(
        old_decl->pos,
        load_function_params_clone(env, old_decl->params),
        rm_tuple_lang_type(env, old_decl->return_type, old_decl->pos),
        old_decl->name
    );
}

static Llvm_variable_def* load_variable_def_clone(Env* env, Tast_variable_def* old_var_def) {
    return llvm_variable_def_new(
        old_var_def->pos,
        rm_tuple_lang_type(env, old_var_def->lang_type, old_var_def->pos),
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
    Lang_type* new_rtn_type,
    Lang_type rtn_type,
    Llvm_block* new_block,
    const Tast_function_params* old_params
) {
    Llvm_function_params* new_params = llvm_function_params_new(
        old_params->pos,
        (Llvm_variable_def_vec) {0},
        0
    );

    Lang_type rtn_lang_type = rm_tuple_lang_type(env, rtn_type, (Pos) {0});
    if (is_struct_like(rtn_type.type)) {
        lang_type_set_pointer_depth(env, &rtn_lang_type, lang_type_get_pointer_depth(rtn_lang_type) + 1);
        Tast_variable_def* new_def = tast_variable_def_new(
            (Pos) {0} /* TODO */,
            rtn_lang_type,
            false,
            util_literal_name_new_prefix("return_as_parameter")
        );
        Llvm_variable_def* param = load_variable_def_clone(env, new_def);
        do_function_def_alloca_param(env, new_params, new_block, param);
        *new_rtn_type = lang_type_void_const_wrap(lang_type_void_new(POS_BUILTIN));
        env->struct_rtn_name_parent_function = vec_at(&new_params->params, 0)->name_self;
    } else {
        *new_rtn_type = rtn_type;
    }

    for (size_t idx = 0; idx < old_params->params.info.count; idx++) {
        Llvm_variable_def* param = load_variable_def_clone(env, vec_at(&old_params->params, idx));
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
    unwrap(label_name.count > 0);
    label->name = label_name;

    if (defer_add_sym) {
        alloca_add_defer(env, llvm_def_wrap(llvm_label_wrap(label)));
    } else {
        unwrap(alloca_add(env, llvm_def_wrap(llvm_label_wrap(label))));
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

    assert(label_name_if_true.count > 0);
    assert(label_name_if_false.count > 0);
    Llvm_cond_goto* cond_goto = llvm_cond_goto_new(
        pos,
        load_operator(env, block, old_oper),
        label_name_if_true,
        label_name_if_false,
        0
    );

    vec_append(&a_main, &block->children, llvm_cond_goto_wrap(cond_goto));
}

static Tast_assignment* for_loop_cond_var_assign_new(Env* env, Str_view sym_name, Pos pos) {
    (void) env;
    (void) sym_name;
    (void) pos;
    Uast_literal* literal = util_uast_literal_new_from_int64_t(1, TOKEN_INT_LITERAL, pos);
    Uast_operator* operator = uast_binary_wrap(uast_binary_new(
        pos,
        uast_symbol_wrap(uast_symbol_new(pos, sym_name, (Ulang_type_vec) {0})),
        uast_literal_wrap(literal),
        BINARY_ADD
    ));
    return util_assignment_new(
        env,
        uast_symbol_wrap(uast_symbol_new(pos, sym_name, (Ulang_type_vec) {0})),
        uast_operator_wrap(operator)
    );
}

static Str_view load_function_call(
    Env* env,
    Llvm_block* new_block,
    Tast_function_call* old_call
) {
    bool rtn_is_struct = is_struct_like(old_call->lang_type.type);

    Strv_vec new_args = {0};

    Str_view def_name = {0};
    Lang_type fun_lang_type = old_call->lang_type;
    if (rtn_is_struct) {
        def_name = util_literal_name_new_prefix("result_fun_call");
        Tast_variable_def* def = tast_variable_def_new(old_call->pos, old_call->lang_type, false, def_name);
        unwrap(sym_tbl_add(&vec_at(&env->ancesters, 0)->symbol_table, tast_variable_def_wrap(def)));
        
        vec_append(&a_main, &new_args, def_name);
        load_variable_def(env, new_block, def);
        fun_lang_type = lang_type_void_const_wrap(lang_type_void_new(POS_BUILTIN));
        //unreachable(TAST_FMT, tast_function_call_print(old_call));
    }

    Llvm_function_call* new_call = llvm_function_call_new(
        old_call->pos,
        new_args,
        util_literal_name_new(),
        load_expr(env, new_block, old_call->callee),
        0,
        fun_lang_type
    );
    unwrap(alloca_add(env, llvm_expr_wrap(llvm_function_call_wrap(new_call))));

    for (size_t idx = 0; idx < old_call->args.info.count; idx++) {
        Tast_expr* old_arg = vec_at(&old_call->args, idx);
        Str_view thing = load_expr(env, new_block, old_arg);
        vec_append(&a_main, &new_call->args, thing);
        Llvm* result = NULL;
        unwrap(alloca_lookup(&result, env, thing));
    }

    vec_append(&a_main, &new_block->children, llvm_expr_wrap(llvm_function_call_wrap(new_call)));

    if (rtn_is_struct) {
        assert(def_name.count > 0);

        Tast_symbol* new_sym = tast_symbol_new(old_call->pos, (Sym_typed_base) {
           .name = def_name,
           .lang_type = old_call->lang_type,
           .llvm_id = 0
        });

        Str_view result = load_expr(env, new_block, tast_symbol_wrap(new_sym));
        return result;
    } else {
        return new_call->name_self;
    }
}

// this function is needed for situations such as switching directly on sum
static Str_view load_ptr_function_call(
    Env* env,
    Llvm_block* new_block,
    Tast_function_call* old_call
) {
    Tast_variable_def* new_var = tast_variable_def_new(
        old_call->pos,
        old_call->lang_type,
        false,
        util_literal_name_new()
    );
    unwrap(symbol_add(env, tast_variable_def_wrap(new_var)));
    load_variable_def(env, new_block, new_var);

    Tast_assignment* new_assign = tast_assignment_new(
        old_call->pos,
        tast_symbol_wrap(tast_symbol_new_from_variable_def(old_call->pos, new_var)),
        tast_function_call_wrap(old_call)
    );
    load_assignment(env, new_block, new_assign);

    return load_ptr_symbol(env, new_block, tast_symbol_new_from_variable_def(old_call->pos, new_var));
}

static Tast_variable_def* load_struct_literal_internal(
    Env* env,
    Llvm_block* new_block,
    Tast_struct_literal* old_lit
) {
    Tast_variable_def* new_var = tast_variable_def_new(
        old_lit->pos,
        old_lit->lang_type,
        false,
        util_literal_name_new_prefix("struct_lit_helper_var")
    );
    unwrap(symbol_add(env, tast_variable_def_wrap(new_var)));
    load_variable_def(env, new_block, new_var);

    Tast_def* struct_def_ = NULL;
    unwrap(symbol_lookup(&struct_def_, env, lang_type_get_str(old_lit->lang_type)));
    Tast_struct_def* struct_def = tast_struct_def_unwrap(struct_def_);

    for (size_t idx = 0; idx < old_lit->members.info.count; idx++) {
        Str_view memb_name = vec_at(&struct_def->base.members, idx)->name;
        Lang_type memb_lang_type = vec_at(&struct_def->base.members, idx)->lang_type;
        Tast_member_access* access = tast_member_access_new(
            old_lit->pos,
            memb_lang_type,
            memb_name,
            tast_symbol_wrap(tast_symbol_new_from_variable_def(old_lit->pos, new_var))
        );

        Tast_assignment* assign = tast_assignment_new(
            access->pos,
            tast_member_access_wrap(access),
            vec_at(&old_lit->members, idx)
        );

        load_assignment(env, new_block, assign);
    }

    return new_var;
}

static Str_view load_ptr_struct_literal(
    Env* env,
    Llvm_block* new_block,
    Tast_struct_literal* old_lit
) {
    Tast_variable_def* new_var = load_struct_literal_internal(env, new_block, old_lit);
    return load_ptr_symbol(env, new_block, tast_symbol_new_from_variable_def(new_var->pos, new_var));
}

static Str_view load_struct_literal(Env* env, Llvm_block* new_block, Tast_struct_literal* old_lit) {
    Tast_variable_def* new_var = load_struct_literal_internal(env, new_block, old_lit);
    return load_symbol(env, new_block, tast_symbol_new_from_variable_def(new_var->pos, new_var));
}

static Str_view load_string(
    Env* env,
    Tast_string* old_lit
) {
    Llvm_string* string = llvm_string_new(
        old_lit->pos,
        old_lit->data,
        old_lit->name
    );
    Tast_string_def* new_def = tast_string_def_new(
        old_lit->pos,
        string->name,
        string->data
    );
    log(LOG_DEBUG, TAST_FMT, tast_string_def_print(new_def));
    unwrap(sym_tbl_add(&vec_at(&env->ancesters, 0)->symbol_table, tast_literal_def_wrap(tast_string_def_wrap(new_def))));
    unwrap(alloca_add(env, llvm_expr_wrap(llvm_literal_wrap(llvm_string_wrap(string)))));
    return string->name;
}

static Str_view load_void(
    Env* env,
    Pos pos
) {
    Llvm_void* new_void = llvm_void_new(pos, util_literal_name_new());
    unwrap(alloca_add(env, llvm_expr_wrap(llvm_literal_wrap(llvm_void_wrap(new_void)))));
    return new_void->name;
}

static Str_view load_enum_lit(
    Env* env,
    Tast_enum_lit* old_lit
) {
    Llvm_enum_lit* enum_lit = llvm_enum_lit_new(
        old_lit->pos,
        old_lit->data,
        old_lit->lang_type,
        util_literal_name_new()
    );
    unwrap(alloca_add(env, llvm_expr_wrap(llvm_literal_wrap(llvm_enum_lit_wrap(enum_lit)))));
    return enum_lit->name;
}

static Str_view load_number(
    Env* env,
    Tast_number* old_lit
) {
    Llvm_number* number = llvm_number_new(
        old_lit->pos,
        old_lit->data,
        old_lit->lang_type,
        util_literal_name_new()
    );
    unwrap(alloca_add(env, llvm_expr_wrap(llvm_literal_wrap(llvm_number_wrap(number)))));
    return number->name;
}

static Str_view load_char(
    Env* env,
    Tast_char* old_lit
) {
    Llvm_char* lang_char = llvm_char_new(old_lit->pos, old_lit->data, util_literal_name_new());
    unwrap(alloca_add(env, llvm_expr_wrap(llvm_literal_wrap(llvm_char_wrap(lang_char)))));
    return lang_char->name;
}

static Str_view load_function_lit(
    Env* env,
    Tast_function_lit* old_lit
) {
    Llvm_function_name* name = llvm_function_name_new(
        old_lit->pos,
        old_lit->name,
        util_literal_name_new()
    );
    unwrap(alloca_add(env, llvm_expr_wrap(llvm_literal_wrap(llvm_function_name_wrap(name)))));
    return name->name_self;
}

static Str_view load_sum_lit(
    Env* env,
    Llvm_block* new_block,
    Tast_sum_lit* old_lit
) {
    Tast_def* sum_def_ = NULL;
    unwrap(symbol_lookup(&sum_def_, env, lang_type_get_str(old_lit->sum_lang_type)));
    //Tast_sum_def* sum_def = tast_sum_def_unwrap(sum_def_);
    Lang_type new_lang_type = rm_tuple_lang_type_sum(
        env,
        lang_type_sum_const_unwrap(old_lit->sum_lang_type),
        old_lit->pos
    );

    Tast_raw_union_def* item_def = get_raw_union_def_from_sum_def(
        env,
        tast_sum_def_unwrap(sum_def_)
    );

    Tast_expr_vec members = {0};
    vec_append(&a_main, &members, tast_literal_wrap(tast_enum_lit_wrap(old_lit->tag)));
    vec_append(&a_main, &members, tast_literal_wrap(tast_raw_union_lit_wrap(
        tast_raw_union_lit_new(
            old_lit->pos,
            old_lit->tag,
            tast_raw_union_def_get_lang_type(item_def),
            old_lit->item
        )
    )));

    return load_struct_literal(env, new_block, tast_struct_literal_new(
        old_lit->pos,
        members,
        util_literal_name_new(),
        new_lang_type
    ));
}

static Str_view load_raw_union_lit(
    Env* env,
    Llvm_block* new_block,
    Tast_raw_union_lit* old_lit
) {
    Tast_def* union_def_ = NULL;
    unwrap(symbol_lookup(&union_def_, env, lang_type_get_str(rm_tuple_lang_type(env, old_lit->lang_type, (Pos) {0}))));
    Tast_raw_union_def* union_def = tast_raw_union_def_unwrap(union_def_);
    Tast_variable_def* active_memb = vec_at(&union_def->base.members, (size_t)old_lit->tag->data);

    Tast_variable_def* new_var = tast_variable_def_new(
        union_def->pos,
        old_lit->lang_type,
        false,
        util_literal_name_new()
    );
    unwrap(symbol_add(env, tast_variable_def_wrap(new_var)));
    load_variable_def(env, new_block, new_var);
    Tast_member_access* access = tast_member_access_new(
        new_var->pos,
        active_memb->lang_type,
        active_memb->name,
        tast_symbol_wrap(tast_symbol_new_from_variable_def(new_var->pos, new_var))
    );

    if (active_memb->lang_type.type != LANG_TYPE_VOID) {
        Tast_assignment* assign = tast_assignment_new(
            new_var->pos,
            tast_member_access_wrap(access),
            old_lit->item
        );
        load_assignment(env, new_block, assign);
    }

    return load_symbol(env, new_block, tast_symbol_new_from_variable_def(new_var->pos, new_var));
}

static Str_view load_literal(
    Env* env,
    Llvm_block* new_block,
    Tast_literal* old_lit
) {
    switch (old_lit->type) {
        case TAST_STRING:
            return load_string(env, tast_string_unwrap(old_lit));
        case TAST_VOID:
            return load_void(env, tast_void_unwrap(old_lit)->pos);
        case TAST_ENUM_LIT:
            return load_enum_lit(env, tast_enum_lit_unwrap(old_lit));
        case TAST_CHAR:
            return load_char(env, tast_char_unwrap(old_lit));
        case TAST_NUMBER:
            return load_number(env, tast_number_unwrap(old_lit));
        case TAST_FUNCTION_LIT:
            return load_function_lit(env, tast_function_lit_unwrap(old_lit));
        case TAST_SUM_LIT:
            return load_sum_lit(env, new_block, tast_sum_lit_unwrap(old_lit));
        case TAST_RAW_UNION_LIT:
            return load_raw_union_lit(env, new_block, tast_raw_union_lit_unwrap(old_lit));
    }
    unreachable("");
}

static Str_view load_ptr_symbol(
    Env* env,
    Llvm_block* new_block,
    Tast_symbol* old_sym
) {
    (void) new_block;

    Tast_def* var_def_ = NULL;
    unwrap(symbol_lookup(&var_def_, env, old_sym->base.name));
    Llvm_variable_def* var_def = load_variable_def_clone(env, tast_variable_def_unwrap(var_def_));
    Llvm* alloca = NULL;
    if (!alloca_lookup(&alloca, env, var_def->name_corr_param)) {
        load_variable_def(env, new_block, tast_variable_def_unwrap(var_def_));
        unwrap(alloca_lookup(&alloca, env, var_def->name_corr_param));
    }

    assert(var_def);

    //Lang_type new_lang_type = rm_tuple_lang_type(env, old_sym->lang_type, old_sym->pos);
    switch (old_sym->base.lang_type.type) {
        case LANG_TYPE_SUM:
            // TODO: should this always be interpreted as loading the tag of the sum?
        case LANG_TYPE_STRUCT:
            // fallthrough
        case LANG_TYPE_ENUM:
            // fallthrough
        case LANG_TYPE_RAW_UNION:
            // fallthrough
        case LANG_TYPE_PRIMITIVE:
            // fallthrough
        case LANG_TYPE_VOID:
            // fallthrough
        case LANG_TYPE_TUPLE:
            // fallthrough
        case LANG_TYPE_FN:
            // fallthrough
        return llvm_tast_get_name(alloca);
    }
    unreachable("");


}

static Str_view load_ptr_sum_callee(
    Env* env,
    Llvm_block* new_block,
    Tast_sum_callee* old_callee
) {
    (void) new_block;
    (void) env;
    (void) old_callee;

    //Tast_def* var_def_ = NULL;
    //unwrap(symbol_lookup(&var_def_, env, old_callee->name));
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

    Str_view ptr = load_ptr_symbol(env, new_block, old_sym);

    Llvm_load_another_llvm* new_load = llvm_load_another_llvm_new(
        pos,
        ptr,
        0,
        rm_tuple_lang_type(env, old_sym->base.lang_type, old_sym->pos),
        util_literal_name_new()
    );
    unwrap(alloca_add(env, llvm_load_another_llvm_wrap(new_load)));

    vec_append(&a_main, &new_block->children, llvm_load_another_llvm_wrap(new_load));
    return new_load->name;
}

static Str_view load_binary_short_circuit(
    Env* env,
    Llvm_block* new_block,
    Tast_binary* old_bin
) {
    BINARY_TYPE if_true_type = {0};
    int if_false_val = 0;

    switch (old_bin->token_type) {
        case BINARY_LOGICAL_AND:
            if_true_type = BINARY_NOT_EQUAL;
            if_false_val = 0;
            break;
        case BINARY_LOGICAL_OR:
            if_true_type = BINARY_DOUBLE_EQUAL;
            if_false_val = 1;
            break;
        default:
            unreachable("");
    }

    Lang_type u1_lang_type = lang_type_primitive_const_wrap(lang_type_unsigned_int_const_wrap(
        lang_type_unsigned_int_new(POS_BUILTIN, 1, 0)
    ));

    Tast_variable_def* new_var = tast_variable_def_new(
        old_bin->pos,
        u1_lang_type,
        false,
        util_literal_name_new_prefix("short_cir")
    );
    unwrap(sym_tbl_add(&vec_at(&env->ancesters, 0)->symbol_table, tast_variable_def_wrap(new_var)));

    Tast_stmt_vec if_true_stmts = {0};
    vec_append(&a_main, &if_true_stmts, tast_expr_wrap(tast_assignment_wrap(tast_assignment_new(
        old_bin->pos,
        tast_symbol_wrap(tast_symbol_new_from_variable_def(old_bin->pos, new_var)),
        old_bin->rhs
    ))));

    Tast_stmt_vec if_false_stmts = {0};
    vec_append(&a_main, &if_false_stmts, tast_expr_wrap(tast_assignment_wrap(tast_assignment_new(
        old_bin->pos,
        tast_symbol_wrap(tast_symbol_new_from_variable_def(old_bin->pos, new_var)),
        tast_literal_wrap(
            util_tast_literal_new_from_int64_t(if_false_val, TOKEN_INT_LITERAL, old_bin->pos)
        )
    ))));

    Tast_if* if_true = tast_if_new(
        old_bin->pos,
        tast_condition_new(old_bin->pos, tast_binary_wrap(tast_binary_new(
            old_bin->pos,
            old_bin->lhs,
            tast_literal_wrap(
                util_tast_literal_new_from_int64_t(0, TOKEN_INT_LITERAL, old_bin->pos)
            ),
            if_true_type,
            u1_lang_type
        ))),
        // TODO: load inner expr in block, and update new symbol
        tast_block_new(
            old_bin->pos,
            if_true_stmts,
            (Symbol_collection) {0},
            old_bin->pos,
            lang_type_void_const_wrap(lang_type_void_new(POS_BUILTIN))
        ),
        lang_type_void_const_wrap(lang_type_void_new(POS_BUILTIN))
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
        tast_block_new(
            old_bin->pos,
            if_false_stmts,
            (Symbol_collection) {0},
            old_bin->pos,
            lang_type_void_const_wrap(lang_type_void_new(POS_BUILTIN))
        ),
        lang_type_void_const_wrap(lang_type_void_new(POS_BUILTIN))
    );

    Tast_if_vec ifs = {0};
    vec_append(&a_main, &ifs, if_true);
    vec_append(&a_main, &ifs, if_false);
    Tast_if_else_chain* if_else = tast_if_else_chain_new(old_bin->pos, ifs, false);
    
    load_variable_def(env, new_block, new_var);
    load_if_else_chain(env, new_block, if_else);
    return load_symbol(env, new_block, tast_symbol_new_from_variable_def(old_bin->pos, new_var));

}

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

    unwrap(alloca_add(env, llvm_expr_wrap(llvm_operator_wrap(llvm_binary_wrap(new_bin)))));

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
            unwrap(alloca_add(env, llvm_load_another_llvm_wrap(new_load)));

            vec_append(&a_main, &new_block->children, llvm_load_another_llvm_wrap(new_load));
            return new_load->name;
        }
        default:
            todo();
    }
    unreachable("");
}

static Str_view load_unary(Env* env, Llvm_block* new_block, Tast_unary* old_unary) {
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
                    if (lang_type_is_equal(tast_expr_get_lang_type(old_unary->child), old_unary->lang_type)) {
                        return load_expr(env, new_block, old_unary->child);
                    }
                    break;
                default:
                    break;
            }

            Str_view new_child = load_expr(env, new_block, old_unary->child);
            if (lang_type_is_equal(old_unary->lang_type, lang_type_from_get_name(env, new_child))) {
                return new_child;
            }

            if (lang_type_get_pointer_depth(old_unary->lang_type) > 0 && lang_type_get_pointer_depth(tast_expr_get_lang_type(old_unary->child)) > 0) {
                return new_child;
            }

            Llvm_unary* new_unary = llvm_unary_new(
                old_unary->pos,
                new_child,
                old_unary->token_type,
                old_unary->lang_type,
                0,
                util_literal_name_new()
            );
            unwrap(alloca_add(env, llvm_expr_wrap(llvm_operator_wrap(llvm_unary_wrap(new_unary)))));

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
    unwrap(symbol_lookup(&def, env, lang_type_get_str(lang_type_from_get_name(env, new_callee))));

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
        lang_type_primitive_const_wrap(lang_type_signed_int_const_wrap(lang_type_signed_int_new(POS_BUILTIN, 64, 0)))
    );
    
    Llvm_load_element_ptr* new_load = llvm_load_element_ptr_new(
        old_access->pos,
        old_access->lang_type,
        0,
        load_literal(env, new_block, tast_number_wrap(new_index)),
        new_callee,
        util_literal_name_new(),
        true
    );
    unwrap(alloca_add(env, llvm_load_element_ptr_wrap(new_load)));

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
        false
    );
    unwrap(alloca_add(env, llvm_load_element_ptr_wrap(new_load)));

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
    unwrap(alloca_add(env, llvm_load_another_llvm_wrap(new_load)));

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
    unwrap(alloca_add(env, llvm_load_another_llvm_wrap(new_load)));

    vec_append(&a_main, &new_block->children, llvm_load_another_llvm_wrap(new_load));
    return new_load->name;
}

static Str_view load_ptr_sum_access_1(
    Env* env,
    Llvm_block* new_block,
    Tast_sum_access* old_access
) {
    (void) env;
    (void) new_block;
    (void) old_access;
    //Tast_def* sum_def_ = NULL;
    //unwrap(symbol_lookup(&sum_def_, env, lang_type_get_str(tast_expr_get_lang_type(old_access->callee))));
    //Tast_sum_def* sum_def = tast_sum_def_unwrap(sum_def_);
    //Str_view new_callee = load_ptr_expr(env, new_block, old_access->callee);
    //Tast_raw_union_def* union_def = get_raw_union_def_from_sum_def( env, sum_def);
    ////Str_view union_name_sum = get_union_name_from_lang_type_sum();
    //
    //Tast_member_access* access = tast_member_access_new(
    //    old_access->pos,
    //    tast_raw_union_def_get_lang_type(union_def),
    //    Str_view member_name,
    //    Tast_expr* callee
    //);

    //Llvm* new_callee_actual = NULL;
    //unwrap(alloca_lookup(&new_callee_actual, env, new_callee));

    //serialize_tast_struct_def(env, old_struct_def);
    todo();
}

static Str_view load_ptr_sum_get_tag(
    Env* env,
    Llvm_block* new_block,
    Tast_sum_get_tag* old_access
) {
    Tast_def* sum_def_ = NULL;
    unwrap(symbol_lookup(&sum_def_, env, lang_type_get_str(tast_expr_get_lang_type(old_access->callee))));
    //Tast_sum_def* sum_def = tast_sum_def_unwrap(sum_def_);
    Str_view new_sum = load_ptr_expr(env, new_block, old_access->callee);
    //Tast_enum_def* union_def = get_enum_def_from_sum_def(env, sum_def);
    
    Tast_number* zero = tast_number_new(
        old_access->pos,
        0,
        lang_type_primitive_const_wrap(lang_type_signed_int_const_wrap(lang_type_signed_int_new(POS_BUILTIN, 64, 0)))
    );

    Llvm_load_element_ptr* new_enum = llvm_load_element_ptr_new(
        old_access->pos,
        lang_type_primitive_const_wrap(lang_type_signed_int_const_wrap(lang_type_signed_int_new(POS_BUILTIN, 64, 0))),
        0,
        load_literal(env, new_block, tast_number_wrap(zero)),
        new_sum,
        util_literal_name_new(),
        true
    );
    unwrap(alloca_add(env, llvm_load_element_ptr_wrap(new_enum)));
    vec_append(&a_main, &new_block->children, llvm_load_element_ptr_wrap(new_enum));

    //Llvm_load_element_ptr* new_item = llvm_load_element_ptr_new(
    //    old_sym->pos,
    //    rm_tuple_lang_type(env, old_sym->base.lang_type, old_sym->pos),
    //    0,
    //    load_literal(env, new_block, tast_number_wrap(zero)),
    //    new_enum->name_self,
    //    util_literal_name_new(),
    //    false
    //);
    //unwrap(alloca_add(env, llvm_load_element_ptr_wrap(new_item)));
    //vec_append(&a_main, &new_block->children, llvm_load_element_ptr_wrap(new_item));

    return new_enum->name_self;
}

static Str_view load_sum_get_tag(
    Env* env,
    Llvm_block* new_block,
    Tast_sum_get_tag* old_access
) {
    Llvm_load_another_llvm* new_load = llvm_load_another_llvm_new(
        old_access->pos,
        load_ptr_sum_get_tag(env, new_block, old_access),
        0,
        lang_type_primitive_const_wrap(lang_type_signed_int_const_wrap(lang_type_signed_int_new(POS_BUILTIN, 64, 0))),
        util_literal_name_new()
    );
    unwrap(alloca_add(env, llvm_load_another_llvm_wrap(new_load)));

    vec_append(&a_main, &new_block->children, llvm_load_another_llvm_wrap(new_load));
    return new_load->name;
}

static Str_view load_ptr_sum_access(
    Env* env,
    Llvm_block* new_block,
    Tast_sum_access* old_access
) {
    Tast_def* sum_def_ = NULL;
    unwrap(symbol_lookup(&sum_def_, env, lang_type_get_str(tast_expr_get_lang_type(old_access->callee))));
    Tast_sum_def* sum_def = tast_sum_def_unwrap(sum_def_);
    Str_view new_callee = load_ptr_expr(env, new_block, old_access->callee);
    Tast_raw_union_def* union_def = get_raw_union_def_from_sum_def(env, sum_def);
    
    Tast_number* zero = tast_number_new(
        old_access->pos,
        0,
        lang_type_primitive_const_wrap(lang_type_signed_int_const_wrap(lang_type_signed_int_new(POS_BUILTIN, 64, 0)))
    );
    Tast_number* one = tast_number_new(
        old_access->pos,
        1,
        lang_type_primitive_const_wrap(lang_type_signed_int_const_wrap(lang_type_signed_int_new(POS_BUILTIN, 64, 0)))
    );

    Llvm_load_element_ptr* new_union = llvm_load_element_ptr_new(
        old_access->pos,
        tast_raw_union_def_get_lang_type(union_def),
        0,
        load_literal(env, new_block, tast_number_wrap(one)),
        new_callee,
        util_literal_name_new(),
        true
    );
    unwrap(alloca_add(env, llvm_load_element_ptr_wrap(new_union)));
    vec_append(&a_main, &new_block->children, llvm_load_element_ptr_wrap(new_union));

    Llvm_load_element_ptr* new_item = llvm_load_element_ptr_new(
        old_access->pos,
        rm_tuple_lang_type(env, old_access->lang_type, old_access->pos),
        0,
        load_literal(env, new_block, tast_number_wrap(zero)),
        new_union->name_self,
        util_literal_name_new(),
        false
    );
    unwrap(alloca_add(env, llvm_load_element_ptr_wrap(new_item)));
    vec_append(&a_main, &new_block->children, llvm_load_element_ptr_wrap(new_item));

    return new_item->name_self;
}

static Str_view load_sum_access(
    Env* env,
    Llvm_block* new_block,
    Tast_sum_access* old_access
) {
    Str_view ptr = load_ptr_sum_access(env, new_block, old_access);

    Llvm_load_another_llvm* new_load = llvm_load_another_llvm_new(
        old_access->pos,
        ptr,
        0,
        lang_type_from_get_name(env, ptr),
        util_literal_name_new()
    );
    unwrap(alloca_add(env, llvm_load_another_llvm_wrap(new_load)));
    vec_append(&a_main, &new_block->children, llvm_load_another_llvm_wrap(new_load));
    return new_load->name;
}

static Str_view load_sum_case(
    Env* env,
    Tast_sum_case* old_case
) {
    return load_enum_lit(env, old_case->tag);
}

static Str_view load_tuple(
    Env* env,
    Llvm_block* new_block,
    Tast_tuple* old_tuple
) {
    Lang_type new_lang_type = lang_type_struct_const_wrap(rm_tuple_lang_type_tuple(
        env, old_tuple->lang_type, old_tuple->pos
    ));
    Str_view new_lit = load_struct_literal(env, new_block, tast_struct_literal_new(
        old_tuple->pos, old_tuple->members, util_literal_name_new(), new_lang_type
    ));

    Llvm* dummy = NULL;
    unwrap(alloca_lookup(&dummy, env, new_lit));
    return new_lit;
}

// TODO: make separate tuple types for lhs and rhs
static Str_view load_tuple_ptr(
    Env* env,
    Llvm_block* new_block,
    Tast_tuple* old_tuple
) {
    (void) new_block;
    Lang_type new_lang_type = lang_type_struct_const_wrap(rm_tuple_lang_type_tuple(
        env, old_tuple->lang_type, old_tuple->pos
    ));
    (void) new_lang_type;

    for (size_t idx = 0; idx < old_tuple->members.info.count; idx++) {
        //Tast_expr* curr_lhs = vec_at(&old_tuple->members, idx);
        todo();
    }

    //Llvm* dummy = NULL;
    //unwrap(alloca_lookup(&dummy, env, new_lit));
    todo();
    //return new_lit;
}

static Str_view load_expr(Env* env, Llvm_block* new_block, Tast_expr* old_expr) {
    switch (old_expr->type) {
        case TAST_ASSIGNMENT:
            return load_assignment(env, new_block, tast_assignment_unwrap(old_expr));
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
        case TAST_SUM_ACCESS:
            return load_sum_access(env, new_block, tast_sum_access_unwrap(old_expr));
        case TAST_SUM_CASE:
            return load_sum_case(env, tast_sum_case_unwrap(old_expr));
        case TAST_STRUCT_LITERAL:
            return load_struct_literal(env, new_block, tast_struct_literal_unwrap(old_expr));
        case TAST_TUPLE:
            return load_tuple(env, new_block, tast_tuple_unwrap(old_expr));
        case TAST_SUM_CALLEE:
            todo();
        case TAST_SUM_GET_TAG:
            return load_sum_get_tag(env, new_block, tast_sum_get_tag_unwrap(old_expr));
        case TAST_IF_ELSE_CHAIN:
            return load_if_else_chain(env, new_block, tast_if_else_chain_unwrap(old_expr));
    }
    unreachable("");
}

static Llvm_function_params* load_function_parameters(
    Env* env,
    Llvm_block* new_fun_body,
    Lang_type* new_lang_type,
    Lang_type rtn_type,
    Tast_function_params* old_params
) {
    Llvm_function_params* new_params = do_function_def_alloca(env, new_lang_type, rtn_type, new_fun_body, old_params);

    for (size_t idx = 0; idx < new_params->params.info.count; idx++) {
        assert(env->ancesters.info.count > 0);
        Llvm_variable_def* param = vec_at(&new_params->params, idx);

        Llvm* dummy = NULL;

        bool is_struct = is_struct_like(param->lang_type.type);

        if (!is_struct) {
            unwrap(alloca_add(env, llvm_def_wrap(llvm_variable_def_wrap(param))));

            Llvm_store_another_llvm* new_store = llvm_store_another_llvm_new(
                param->pos,
                param->name_self,
                param->name_corr_param,
                0,
                param->lang_type,
                util_literal_name_new()
            );
            unwrap(alloca_add(env, llvm_store_another_llvm_wrap(new_store)));

            vec_append(&a_main, &new_fun_body->children, llvm_store_another_llvm_wrap(new_store));
        }

        unwrap(alloca_lookup(&dummy, env, param->name_corr_param));
    }

    return new_params;
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
        old_fun_def->decl->return_type,
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

    //unwrap(old_fun_def->decl->return_type->lang_type.info.count == 1);

    vec_append(&a_main, &env->ancesters, &new_fun_def->body->symbol_collection);
    Lang_type new_lang_type = {0};
    new_fun_def->decl->params = load_function_parameters(
        env, new_fun_def->body, &new_lang_type, old_fun_def->decl->return_type, old_fun_def->decl->params
    );
    new_fun_def->decl->return_type = new_lang_type;
    for (size_t idx = 0; idx < old_fun_def->body->children.info.count; idx++) {
        load_stmt(env, new_fun_def->body, vec_at(&old_fun_def->body->children, idx));
    }
    vec_rem_last(&env->ancesters);

    unwrap(alloca_add(env, llvm_def_wrap(llvm_function_def_wrap(new_fun_def))));
    env->name_parent_function = old_fun_name;
    return (Str_view) {0};
}

static Str_view load_function_decl(
    Env* env,
    Tast_function_decl* old_fun_decl
) {
    unwrap(alloca_add(env, llvm_def_wrap(llvm_function_decl_wrap(load_function_decl_clone(env, old_fun_decl)))));

    return (Str_view) {0};
}

static Str_view load_return(
    Env* env,
    Llvm_block* new_block,
    Tast_return* old_return
) {
    Pos pos = old_return->pos;

    Tast_def* fun_def_ = NULL;
    unwrap(symbol_lookup(&fun_def_, env, env->name_parent_function));

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
    Lang_type rtn_type = rm_tuple_lang_type(env, fun_decl->return_type, fun_decl->pos);

    bool rtn_is_struct = is_struct_like(rtn_type.type);

    if (rtn_is_struct) {
        Llvm* dest_ = NULL;
        unwrap(alloca_lookup(&dest_, env, env->struct_rtn_name_parent_function));
        Str_view dest = env->struct_rtn_name_parent_function;
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
    Tast_assignment* old_assign
) {
    assert(old_assign->lhs);
    assert(old_assign->rhs);

    Pos pos = old_assign->pos;

    Str_view new_lhs = load_ptr_expr(env, new_block, old_assign->lhs);
    Str_view new_rhs = load_expr(env, new_block, old_assign->rhs);

    Llvm_store_another_llvm* new_store = llvm_store_another_llvm_new(
        pos,
        new_rhs,
        new_lhs,
        0,
        rm_tuple_lang_type(env, tast_expr_get_lang_type(old_assign->lhs), old_assign->pos),
        util_literal_name_new()
    );
    unwrap(alloca_add(env, llvm_store_another_llvm_wrap(new_store)));

    assert(new_store->llvm_src.count > 0);
    assert(new_store->llvm_dest.count > 0);
    assert(old_assign->rhs);

    vec_append(&a_main, &new_block->children, llvm_store_another_llvm_wrap(new_store));

    return new_store->name;
}

static Str_view load_variable_def(
    Env* env,
    Llvm_block* new_block,
    Tast_variable_def* old_var_def
) {
    Llvm_variable_def* new_var_def = load_variable_def_clone(env, old_var_def);

    Llvm* alloca = NULL;
    if (!alloca_lookup(&alloca, env, new_var_def->name_self)) {
        alloca = llvm_alloca_wrap(add_load_and_store_alloca_new(env, new_var_def));
        vec_insert(&a_main, &new_block->children, 0, alloca);
    }

    vec_append(&a_main, &new_block->children, llvm_def_wrap(llvm_variable_def_wrap(new_var_def)));

    assert(alloca);
    return llvm_tast_get_name(alloca);
}

static void load_struct_def(
    Env* env,
    Tast_struct_def* old_def
) {
    all_tbl_add(&vec_at(&env->ancesters, 0)->alloca_table, llvm_def_wrap(
        llvm_struct_def_wrap(load_struct_def_clone(old_def))
    ));

    Tast_def* dummy = NULL;
    if (!symbol_lookup(&dummy, env, serialize_tast_struct_def(env, old_def))) {
        Tast_struct_def* new_def = tast_struct_def_new(old_def->pos, (Struct_def_base) {
            .members = old_def->base.members, 
            .name = serialize_tast_struct_def(env, old_def)
        });
        unwrap(sym_tbl_add(&vec_at(&env->ancesters, 0)->symbol_table, tast_struct_def_wrap(new_def)));
        load_struct_def(env, new_def);
    }
}

static void load_enum_def(
    Env* env,
    Tast_enum_def* old_def
) {
    (void) env;

    all_tbl_add(&vec_at(&env->ancesters, 0)->alloca_table, llvm_def_wrap(
        llvm_enum_def_wrap(load_enum_def_clone(old_def))
    ));
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


    assert(after_chain.count > 0);
    Llvm_goto* jmp_to_after_chain = llvm_goto_new(
        old_block->pos,
        after_chain,
        0
    );
    vec_append(&a_main, &new_block->children, llvm_goto_wrap(jmp_to_after_chain));

    vec_rem_last(&env->ancesters);
    return new_block;
}

static Str_view if_else_chain_to_branch(Llvm_block** new_block, Env* env, Tast_if_else_chain* if_else) {
    *new_block = llvm_block_new(
        if_else->pos,
        (Llvm_vec) {0},
        (Symbol_collection) {0},
        (Pos) {0}
    );

    Tast_variable_def* yield_dest = NULL;
    if (tast_if_else_chain_get_lang_type(if_else).type != LANG_TYPE_VOID) {
        yield_dest = tast_variable_def_new(
            (*new_block)->pos,
            tast_if_else_chain_get_lang_type(if_else),
            false,
            util_literal_name_new()
        );
        unwrap(symbol_add(env, tast_variable_def_wrap(yield_dest)));
        load_variable_def(env, *new_block, yield_dest);
        env->load_break_symbol_name = yield_dest->name;
    }

    Str_view if_after = util_literal_name_new_prefix("if_after");

    Str_view old_label_if_break = env->label_if_break;
    if (if_else->is_switch) {
        env->label_if_break = if_after;
    } else {
        env->label_if_break = env->label_after_for;
    }
    
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
        vec_append(&a_main, &(*new_block)->children, llvm_block_wrap(if_block));

        if (idx + 1 < if_else->tasts.info.count) {
            assert(!alloca_lookup(&dummy, env, next_if));
            add_label(env, (*new_block), next_if, vec_at(&if_else->tasts, idx)->pos, false);
            assert(alloca_lookup(&dummy, env, next_if));
        } else {
            log(LOG_DEBUG, TAST_FMT"\n", str_view_print(next_if));
            //assert(str_view_is_equal(next_if, env->label_if_break));
        }
    }

    assert(!symbol_lookup(&dummy_def, env, next_if));
    add_label(env, (*new_block), next_if, if_else->pos, false);
    assert(alloca_lookup(&dummy, env, next_if));

    env->label_if_break = old_label_if_break;

    if (tast_if_else_chain_get_lang_type(if_else).type == LANG_TYPE_VOID) {
        return (Str_view) {0};
    } else {
        return load_symbol(env, *new_block, tast_symbol_new_from_variable_def(yield_dest->pos, yield_dest));
    }
    unreachable("");
}

static Str_view load_if_else_chain(
    Env* env,
    Llvm_block* new_block,
    Tast_if_else_chain* old_if_else
) {
    Llvm_block* new_if_else = NULL;
    Str_view result = if_else_chain_to_branch(&new_if_else, env, old_if_else);
    vec_append(&a_main, &new_block->children, llvm_block_wrap(new_if_else));

    return result;
}

static Llvm_block* for_with_cond_to_branch(Env* env, Tast_for_with_cond* old_for) {
    Str_view old_after_for = env->label_after_for;
    Str_view old_if_continue = env->label_if_continue;
    Str_view old_if_break = env->label_if_break;

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

    env->label_after_for = after_for_loop_label;
    env->label_if_break = after_for_loop_label;

    if (old_for->do_cont_label) {
        env->label_if_continue = old_for->continue_label;
    } else {
        env->label_if_continue = check_cond_label;
    }

    vec_append(&a_main, &new_branch_block->children, llvm_goto_wrap(jmp_to_check_cond_label));

    add_label(env, new_branch_block, check_cond_label, pos, true);

    //load_operator(env, new_branch_block, operator);
    //Tast* dummy = NULL;
    //unwrap(symbol_lookup(&dummy, env, str_view_from_cstr("str18")));

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

    //unwrap(symbol_lookup(&dummy, env, str_view_from_cstr("str18")));
    //for (size_t idx = 0; idx < old_for->body->children.info.count; idx++) {
    //    log(LOG_DEBUG, TAST_FMT"\n", tast_print(vec_at(&old_for->body->children, idx)));
    //}

    for (size_t idx = 0; idx < old_for->body->children.info.count; idx++) {
        //for (size_t idx = 0; idx < new_branch_block->children.info.count; idx++) {
        //    log(LOG_DEBUG, TAST_FMT"\n", tast_print(vec_at(&new_branch_block->children, idx)));
        //}
        load_stmt(env, new_branch_block, vec_at(&old_for->body->children, idx));
    }

    for (size_t idx = 0; idx < new_branch_block->children.info.count; idx++) {
        //log(LOG_DEBUG, TAST_FMT"", llvm_print(vec_at(&new_branch_block->children, idx)));
    }

    vec_append(&a_main, &new_branch_block->children, llvm_goto_wrap(
        llvm_goto_new(old_for->pos, check_cond_label, 0)
    ));
    add_label(env, new_branch_block, after_for_loop_label, pos, true);

    Llvm* dummy = NULL;

    env->label_if_continue = old_if_continue;
    env->label_after_for = old_after_for;
    env->label_if_break = old_if_break;
    for (size_t idx = 0; idx < env->defered_allocas_to_add.info.count; idx++) {
        //log(LOG_DEBUG, TAST_FMT, llvm_print(vec_at(&env->defered_allocas_to_add, idx)));
    }
    //log_env(LOG_DEBUG, env);
    vec_rem_last(&env->ancesters);
    Symbol_collection* popped = NULL;
    vec_pop(popped, &env->ancesters);
    unwrap(alloca_do_add_defered(&dummy, env));
    vec_append(&a_main, &env->ancesters, popped);

    return new_branch_block;
}

static void load_for_with_cond(
    Env* env,
    Llvm_block* new_block,
    Tast_for_with_cond* old_for
) {
    Llvm_block* new_for = for_with_cond_to_branch(env, old_for);
    vec_append(&a_main, &new_block->children, llvm_block_wrap(new_for));
}

static void load_break(
    Env* env,
    Llvm_block* new_block,
    Tast_break* old_break
) {
    if (env->label_if_break.count < 1) {
        msg(
            LOG_ERROR, EXPECT_FAIL_BREAK_INVALID_LOCATION, env->file_text, old_break->pos,
            "break statement outside of a for loop\n"
        );
        return;
    }

    if (old_break->do_break_expr) {
        load_assignment(env, new_block, tast_assignment_new(
            old_break->pos,
            tast_symbol_wrap(tast_symbol_new(old_break->pos, (Sym_typed_base) {
                .lang_type = tast_expr_get_lang_type(old_break->break_expr),
                .name = env->load_break_symbol_name,
                .llvm_id = 0
            })),
            old_break->break_expr
        ));
    }

    assert(env->label_if_break.count > 0);
    Llvm_goto* new_goto = llvm_goto_new(old_break->pos, env->label_if_break, 0);
    vec_append(&a_main, &new_block->children, llvm_goto_wrap(new_goto));
}

static void load_label(
    Env* env,
    Llvm_block* new_block,
    Tast_label* old_label
) {
    Llvm_label* new_label = llvm_label_new(old_label->pos, 0, old_label->name);
    vec_append(&a_main, &new_block->children, llvm_def_wrap(llvm_label_wrap(new_label)));
    assert(new_label->name.count > 0);
    alloca_add(env, llvm_def_wrap(llvm_label_wrap(new_label)));
}

static void load_continue(
    Env* env,
    Llvm_block* new_block,
    Tast_continue* old_continue
) {
    if (env->label_if_continue.count < 1) {
        msg(
            LOG_ERROR, EXPECT_FAIL_CONTINUE_INVALID_LOCATION, env->file_text, old_continue->pos,
            "continue statement outside of a for loop\n"
        );
        return;
    }

    Llvm_goto* new_goto = llvm_goto_new(old_continue->pos, env->label_if_continue, 0);
    vec_append(&a_main, &new_block->children, llvm_goto_wrap(new_goto));
}

static void load_raw_union_def(
    Env* env,
    Tast_raw_union_def* old_def
) {
    // TODO: crash if alloca_add fails (we need to prevent duplicates to crash on alloca_add fail)?
    if (!all_tbl_add(&vec_at(&env->ancesters, 0)->alloca_table, llvm_def_wrap(
        llvm_raw_union_def_wrap(load_raw_union_def_clone(old_def))
    ))) {
        return;
    };

    Tast_def* dummy = NULL;
    if (!symbol_lookup(&dummy, env, serialize_tast_raw_union_def(env, old_def))) {
        Tast_raw_union_def* new_def = tast_raw_union_def_new(old_def->pos, (Struct_def_base) {
            .members = old_def->base.members, 
            .name = serialize_tast_raw_union_def(env, old_def)
        });
        unwrap(sym_tbl_add(&vec_at(&env->ancesters, 0)->symbol_table, tast_raw_union_def_wrap(new_def)));
        load_raw_union_def(env, new_def);
    }
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
    unwrap(alloca_add(env, llvm_load_another_llvm_wrap(new_load)));
    lang_type_set_pointer_depth(env, &new_load->lang_type, lang_type_get_pointer_depth(new_load->lang_type) + 1);

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
            // TODO: deref string literal (detect in src/type_checking.c if possible)
            unreachable("");
        case TAST_FUNCTION_CALL:
            return load_ptr_function_call(env, new_block, tast_function_call_unwrap(old_expr));
        case TAST_STRUCT_LITERAL:
            return load_ptr_struct_literal(env, new_block, tast_struct_literal_unwrap(old_expr));
        case TAST_TUPLE:
            unreachable("");
        case TAST_SUM_CALLEE:
            return load_ptr_sum_callee(env, new_block, tast_sum_callee_unwrap(old_expr));
        case TAST_SUM_CASE:
            unreachable("");
        case TAST_SUM_ACCESS:
            unreachable("");
        case TAST_ASSIGNMENT:
            unreachable("");
        case TAST_SUM_GET_TAG:
            return load_ptr_sum_get_tag(env, new_block, tast_sum_get_tag_unwrap(old_expr));
        case TAST_IF_ELSE_CHAIN:
            todo();
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
            load_struct_def(env, tast_struct_def_unwrap(old_def));
            return (Str_view) {0};
        case TAST_ENUM_DEF:
            load_enum_def(env, tast_enum_def_unwrap(old_def));
            return (Str_view) {0};
        case TAST_RAW_UNION_DEF:
            load_raw_union_def(env, tast_raw_union_def_unwrap(old_def));
            return (Str_view) {0};
        case TAST_SUM_DEF:
            unreachable("sum def should not make it here");
        case TAST_LITERAL_DEF:
            unreachable("");
        case TAST_PRIMITIVE_DEF:
            unreachable("");
        case TAST_POISON_DEF:
            unreachable("compilation should have exited before now if a poison def was generated");
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
        case TAST_FOR_WITH_COND:
            load_for_with_cond(env, new_block, tast_for_with_cond_unwrap(old_stmt));
            return (Str_view) {0};
        case TAST_BREAK:
            load_break(env, new_block, tast_break_unwrap(old_stmt));
            return (Str_view) {0};
        case TAST_CONTINUE:
            load_continue(env, new_block, tast_continue_unwrap(old_stmt));
            return (Str_view) {0};
        case TAST_BLOCK:
            vec_append(
                &a_main,
                &new_block->children,
                llvm_block_wrap(load_block(env, tast_block_unwrap(old_stmt)))
            );
            return (Str_view) {0};
        case TAST_LABEL:
            load_label(env, new_block, tast_label_unwrap(old_stmt));
            return (Str_view) {0};
    }
    unreachable("");
}

static Str_view load_tast(Env* env, Llvm_block* new_block, Tast* old_tast) {
    (void) env;
    (void) new_block;
    (void) old_tast;
    unreachable(TAST_FMT"\n", tast_print(old_tast));
}

static Str_view load_def_sometimes(Env* env, Llvm_block* new_block, Tast_def* old_def) {
    (void) new_block;
    switch (old_def->type) {
        case TAST_FUNCTION_DEF:
            return load_function_def(env, tast_function_def_unwrap(old_def));
        case TAST_FUNCTION_DECL:
            return load_function_decl(env, tast_function_decl_unwrap(old_def));
        case TAST_VARIABLE_DEF:
            //return load_variable_def(env, new_block, tast_variable_def_unwrap(old_def));
            return (Str_view) {0};
        case TAST_STRUCT_DEF:
            load_struct_def(env, tast_struct_def_unwrap(old_def));
            return (Str_view) {0};
        case TAST_ENUM_DEF:
            load_enum_def(env, tast_enum_def_unwrap(old_def));
            return (Str_view) {0};
        case TAST_RAW_UNION_DEF:
            load_raw_union_def(env, tast_raw_union_def_unwrap(old_def));
            return (Str_view) {0};
        case TAST_SUM_DEF:
            return (Str_view) {0};
        case TAST_LITERAL_DEF:
            return (Str_view) {0};
        case TAST_PRIMITIVE_DEF:
            unreachable("");
        case TAST_POISON_DEF:
            unreachable("");
    }
    unreachable("");
}

static Llvm_block* load_block(Env* env, Tast_block* old_block) {
    size_t init_count_ancesters = env->ancesters.info.count;

    Llvm_block* new_block = llvm_block_new(
        old_block->pos,
        (Llvm_vec) {0},
        old_block->symbol_collection,
        old_block->pos_end
    );

    vec_append(&a_main, &env->ancesters, &new_block->symbol_collection);

    Symbol_iter iter = sym_tbl_iter_new(vec_top(&env->ancesters)->symbol_table);
    Tast_def* curr = NULL;
    while (sym_tbl_iter_next(&curr, &iter)) {
        log(LOG_DEBUG, TAST_FMT, tast_def_print(curr));
        load_def_sometimes(env, new_block, curr);
    }

    for (size_t idx = 0; idx < old_block->children.info.count; idx++) {
        load_stmt(env, new_block, vec_at(&old_block->children, idx));
    }

    vec_rem_last(&env->ancesters);

    unwrap(init_count_ancesters == env->ancesters.info.count);
    return new_block;
}

Llvm_block* add_load_and_store(Env* env, Tast_block* old_root) {
    Llvm_block* block = load_block(env, old_root);
    return block;
}

