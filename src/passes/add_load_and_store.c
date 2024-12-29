#include <node.h>
#include <llvm.h>
#include <nodes.h>
#include <symbol_table.h>
#include <parser_utils.h>
#include <log_env.h>

#include "passes.h"

static Llvm_variable_def* node_clone_variable_def(Node_variable_def* old_var_def);

static Llvm_alloca* add_load_and_store_alloca_new(Env* env, Llvm_variable_def* var_def) {
    Llvm_alloca* alloca = llvm_alloca_new(var_def->pos, 0, var_def->lang_type, var_def->name_corr_param);
    alloca_add(env, llvm_wrap_alloca(alloca));
    assert(alloca);
    return alloca;
}

static Llvm_function_params* node_clone_function_params(Node_function_params* old_params) {
    Llvm_function_params* new_params = llvm_function_params_new(old_params->pos, (Llvm_variable_def_vec){0}, 0);

    for (size_t idx = 0; idx < old_params->params.info.count; idx++) {
        vec_append(&a_main, &new_params->params, node_clone_variable_def(vec_at(&old_params->params, idx)));
    }

    return new_params;
}

static Llvm_lang_type* node_clone_lang_type(Node_lang_type* old_lang_type) {
    Llvm_lang_type* new_lang_type = llvm_lang_type_new(old_lang_type->pos, old_lang_type->lang_type);
    return new_lang_type;
}

static Llvm_function_decl* node_clone_function_decl(Node_function_decl* old_decl) {
    return llvm_function_decl_new(
        old_decl->pos,
        node_clone_function_params(old_decl->params),
        node_clone_lang_type(old_decl->return_type),
        old_decl->name
    );
}

static Llvm_variable_def* node_clone_variable_def(Node_variable_def* old_var_def) {
    return llvm_variable_def_new(
        old_var_def->pos,
        old_var_def->lang_type,
        old_var_def->is_variadic,
        0,
        util_literal_name_new(),
        old_var_def->name
    );
}

static Llvm_struct_literal* node_clone_struct_literal(const Node_struct_literal* old_lit) {
    return llvm_struct_literal_new(
        old_lit->pos,
        old_lit->members,
        old_lit->name,
        old_lit->lang_type,
        0
    );
}

static Llvm_struct_def* node_clone_struct_def(const Node_struct_def* old_def) {
    return llvm_struct_def_new(
        old_def->pos,
        old_def->base
    );
}

static Llvm_enum_def* node_clone_enum_def(const Node_enum_def* old_def) {
    return llvm_enum_def_new(
        old_def->pos,
        old_def->base
    );
}

static Llvm_raw_union_def* node_clone_raw_union_def(const Node_raw_union_def* old_def) {
    return llvm_raw_union_def_new(
        old_def->pos,
        old_def->base
    );
}

static Llvm_function_params* do_function_def_alloca(Env* env, Llvm_block* new_block, const Node_function_params* old_params) {
    Llvm_function_params* new_params = llvm_function_params_new(
        old_params->pos,
        (Llvm_variable_def_vec) {0},
        0
    );

    for (size_t idx = 0; idx < old_params->params.info.count; idx++) {
        Llvm_variable_def* param = node_clone_variable_def(vec_at(&old_params->params, idx));

        if (lang_type_is_struct(env, param->lang_type)) {
            param->name_self = param->name_corr_param;
            alloca_add(env, llvm_wrap_def(llvm_wrap_variable_def(param)));
        } else if (lang_type_is_enum(env, param->lang_type)) {
            vec_insert(&a_main, &new_block->children, 0, llvm_wrap_alloca(
                add_load_and_store_alloca_new(env, param)
            ));
        } else if (lang_type_is_raw_union(env, param->lang_type)) {
            param->name_self = param->name_corr_param;
            alloca_add(env, llvm_wrap_def(llvm_wrap_variable_def(param)));
        } else if (lang_type_is_primitive(env, param->lang_type)) {
            vec_insert(&a_main, &new_block->children, 0, llvm_wrap_alloca(
                add_load_and_store_alloca_new(env, param)
            ));
        } else {
            todo();
        }

        vec_append(&a_main, &new_params->params, param);
    }

    return new_params;
}

static Llvm_block* load_block(Env* env, Node_block* old_block);

static Str_view load_expr(Env* env, Llvm_block* new_block, Node_expr* old_expr);

static Str_view load_ptr(Env* env, Llvm_block* new_block, Node* old_node);

static Str_view load_ptr_expr(Env* env, Llvm_block* new_block, Node_expr* old_expr);

static Str_view load_node(Env* env, Llvm_block* new_block, Node* old_node);

static Str_view load_operator(
    Env* env,
    Llvm_block* new_block,
    Node_operator* old_oper
);

static void add_label(Env* env, Llvm_block* block, Str_view label_name, Pos pos, bool defer_add_sym) {
    Llvm_label* label = llvm_label_new(pos, 0, label_name);
    label->name = label_name;

    if (defer_add_sym) {
        alloca_add_defer(env, llvm_wrap_def(llvm_wrap_label(label)));
    } else {
        try(alloca_add(env, llvm_wrap_def(llvm_wrap_label(label))));
    }

    vec_append(&a_main, &block->children, llvm_wrap_def(llvm_wrap_label(label)));
}

static void if_for_add_cond_goto(
    Env* env,
    Node_operator* old_oper,
    Llvm_block* block,
    Str_view label_name_if_true,
    Str_view label_name_if_false
) {
    Pos pos = node_get_pos_operator(old_oper);

    Llvm_cond_goto* cond_goto = llvm_cond_goto_new(
        pos,
        load_operator(env, block, old_oper),
        label_name_if_true,
        label_name_if_false,
        0
    );
    log(LOG_DEBUG, NODE_FMT"\n", llvm_cond_goto_print(cond_goto));

    vec_append(&a_main, &block->children, llvm_wrap_cond_goto(cond_goto));
}

static Node_assignment* for_loop_cond_var_assign_new(Env* env, Str_view sym_name, Pos pos) {
    Node_literal* literal = util_literal_new_from_int64_t(1, TOKEN_INT_LITERAL, pos);
    Node_operator* operator = util_binary_typed_new(
        env,
        node_wrap_symbol_untyped(node_symbol_untyped_new(pos, sym_name)),
        node_wrap_literal(literal),
        TOKEN_SINGLE_PLUS
    );
    return util_assignment_new(
        env,
        node_wrap_expr(node_wrap_symbol_untyped(node_symbol_untyped_new(pos, sym_name))),
        node_wrap_operator(operator)
    );
}

static Str_view load_function_call(
    Env* env,
    Llvm_block* new_block,
    Node_function_call* old_fun_call
) {

    Llvm_function_call* new_fun_call = llvm_function_call_new(
        old_fun_call->pos,
        (Strv_vec) {0},
        util_literal_name_new(),
        old_fun_call->name,
        0,
        old_fun_call->lang_type
    );
    try(alloca_add(env, llvm_wrap_expr(llvm_wrap_function_call(new_fun_call))));

    for (size_t idx = 0; idx < old_fun_call->args.info.count; idx++) {
        Node_expr* old_arg = vec_at(&old_fun_call->args, idx);
        vec_append(&a_main, &new_fun_call->args, load_expr(env, new_block, old_arg));
    }

    vec_append(&a_main, &new_block->children, llvm_wrap_expr(llvm_wrap_function_call(new_fun_call)));
    return new_fun_call->name_self;
}

static Llvm_literal* node_literal_clone(Node_literal* old_lit) {
    Llvm_literal* new_lit = NULL;

    switch (old_lit->type) {
        case NODE_STRING: {
            Llvm_string* string = llvm_string_new(
                node_get_pos_literal(old_lit),
                node_unwrap_string(old_lit)->data,
                node_unwrap_string(old_lit)->lang_type,
                node_unwrap_string(old_lit)->name
            );
            new_lit = llvm_wrap_string(string);
            break;
        }
        case NODE_VOID: {
            new_lit = llvm_wrap_void(llvm_void_new(node_get_pos_literal(old_lit), util_literal_name_new()));
            break;
        }
        case NODE_ENUM_LIT: {
            Llvm_enum_lit* enum_lit = llvm_enum_lit_new(
                node_get_pos_literal(old_lit),
                node_unwrap_enum_lit(old_lit)->data,
                node_unwrap_enum_lit(old_lit)->lang_type,
                util_literal_name_new()
            );
            new_lit = llvm_wrap_enum_lit(enum_lit);
            break;
        }
        case NODE_CHAR: {
            Llvm_char* lang_char = llvm_char_new(
                node_get_pos_literal(old_lit),
                node_unwrap_char(old_lit)->data,
                node_unwrap_char(old_lit)->lang_type,
                util_literal_name_new()
            );
            new_lit = llvm_wrap_char(lang_char);
            break;
        }
        case NODE_NUMBER: {
            Llvm_number* number = llvm_number_new(
                node_get_pos_literal(old_lit),
                node_unwrap_number(old_lit)->data,
                node_unwrap_number(old_lit)->lang_type,
                util_literal_name_new()
            );
            new_lit = llvm_wrap_number(number);
            break;
        }
        default:
            unreachable(NODE_FMT"\n", node_print((Node*)old_lit));
    }

    return new_lit;
}

static Str_view load_literal(
    Env* env,
    Llvm_block* new_block,
    Node_literal* old_lit
) {
    (void) env;
    (void) new_block;
    Pos pos = node_get_pos_literal(old_lit);

    Llvm_literal* new_lit = node_literal_clone(old_lit);

    if (new_lit->type == LLVM_STRING) {
        Node_string_def* new_def = node_string_def_new(
            pos,
            node_unwrap_string(old_lit)->name,
            node_unwrap_string(old_lit)->data
        );
        try(sym_tbl_add(&env->global_literals, node_wrap_literal_def(node_wrap_string_def(new_def))));
    }

    try(alloca_add(env, llvm_wrap_expr(llvm_wrap_literal(new_lit))));

    return llvm_get_literal_name(new_lit);
}

static Str_view load_ptr_symbol_typed(
    Env* env,
    Llvm_block* new_block,
    Node_symbol_typed* old_sym
) {
    (void) new_block;
    log(LOG_DEBUG, "entering thing\n");

    Node_def* var_def_ = NULL;
    try(symbol_lookup(&var_def_, env, get_symbol_typed_name(old_sym)));
    Llvm_variable_def* var_def = node_clone_variable_def(node_unwrap_variable_def(var_def_));
    Llvm* alloca = NULL;
    if (!alloca_lookup(&alloca, env, var_def->name_corr_param)) {
        log(LOG_DEBUG, STR_VIEW_FMT"\n", str_view_print(var_def->name_corr_param));
        unreachable("");
    }

    assert(var_def);
    log(LOG_DEBUG, NODE_FMT"\n", llvm_variable_def_print(var_def));
    log(LOG_DEBUG, NODE_FMT"\n", node_symbol_typed_print(old_sym));
    log(LOG_DEBUG, NODE_FMT"\n", llvm_print(alloca));
    //log(LOG_DEBUG, NODE_FMT"\n", node_print(var_def->storage_location.node));
    assert(lang_type_is_equal(var_def->lang_type, node_get_lang_type_symbol_typed(old_sym)));

    return llvm_get_node_name(alloca);
}

static Str_view load_symbol_typed(
    Env* env,
    Llvm_block* new_block,
    Node_symbol_typed* old_sym
) {
    Pos pos = node_get_pos_symbol_typed(old_sym);

    Llvm_load_another_llvm* new_load = llvm_load_another_llvm_new(
        pos,
        load_ptr_symbol_typed(env, new_block, old_sym),
        0,
        node_get_lang_type_symbol_typed(old_sym),
        util_literal_name_new()
    );
    try(alloca_add(env, llvm_wrap_load_another_llvm(new_load)));

    vec_append(&a_main, &new_block->children, llvm_wrap_load_another_llvm(new_load));
    return new_load->name;
}

static Str_view load_binary(
    Env* env,
    Llvm_block* new_block,
    Node_binary* old_bin
) {
    Llvm_binary* new_bin = llvm_binary_new(
        old_bin->pos,
        load_expr(env, new_block, old_bin->lhs),
        load_expr(env, new_block, old_bin->rhs),
        old_bin->token_type,
        old_bin->lang_type,
        0,
        util_literal_name_new()
    );
    try(alloca_add(env, llvm_wrap_expr(llvm_wrap_operator(llvm_wrap_binary(new_bin)))));

    vec_append(&a_main, &new_block->children, llvm_wrap_expr(llvm_wrap_operator(llvm_wrap_binary(new_bin))));
    return new_bin->name;
}

static Str_view load_unary(
    Env* env,
    Llvm_block* new_block,
    Node_unary* old_unary
) {
    switch (old_unary->token_type) {
        case TOKEN_DEREF: {
            if (lang_type_is_struct(env, node_get_lang_type_expr(old_unary->child))) {
                todo();
            } else if (lang_type_is_primitive(env, node_get_lang_type_expr(old_unary->child))) {
                Str_view ptr = load_expr(env, new_block, old_unary->child);
                Llvm_load_another_llvm* new_load = llvm_load_another_llvm_new(
                    old_unary->pos,
                    ptr,
                    0,
                    old_unary->lang_type,
                    util_literal_name_new()
                );
                try(alloca_add(env, llvm_wrap_load_another_llvm(new_load)));

                vec_append(&a_main, &new_block->children, llvm_wrap_load_another_llvm(new_load));
                return new_load->name;
            } else {
                unreachable("");
            }
        }
        case TOKEN_REFER: {
            return load_ptr_expr(env, new_block, old_unary->child);
        }
        case TOKEN_UNSAFE_CAST:
            if (old_unary->lang_type.pointer_depth > 0 && node_get_lang_type_expr(old_unary->child).pointer_depth > 0) {
                return load_expr(env, new_block, old_unary->child);
            }
            // fallthrough
        case TOKEN_NOT: {
            Llvm_unary* new_unary = llvm_unary_new(
                old_unary->pos,
                load_expr(env, new_block, old_unary->child),
                old_unary->token_type,
                old_unary->lang_type,
                0,
                util_literal_name_new()
            );
            try(alloca_add(env, llvm_wrap_expr(llvm_wrap_operator(llvm_wrap_unary(new_unary)))));

            vec_append(&a_main, &new_block->children, llvm_wrap_expr(llvm_wrap_operator(llvm_wrap_unary(new_unary))));
            return new_unary->name;
        }
        default:
            unreachable(
                NODE_FMT " %d\n",
                node_print(node_wrap_expr(node_wrap_operator(node_wrap_unary(old_unary)))),
                old_unary->token_type
            );
    }
}

static Str_view load_operator(
    Env* env,
    Llvm_block* new_block,
    Node_operator* old_oper
) {
    switch (old_oper->type) {
        case NODE_BINARY:
            return load_binary(env, new_block, node_unwrap_binary(old_oper));
        case NODE_UNARY:
            return load_unary(env, new_block, node_unwrap_unary(old_oper));
    }
    unreachable("");
}

static Str_view load_ptr_member_access_typed(
    Env* env,
    Llvm_block* new_block,
    Node_member_access_typed* old_access
) {
    Str_view new_callee = load_ptr_expr(env, new_block, old_access->callee);

    Node_def* def = NULL;
    try(symbol_lookup(&def, env, get_lang_type_from_name(env, new_callee).str));

    log_tree(LOG_DEBUG, node_wrap_def(def));

    Struct_def_base def_base = {0};
    switch (def->type) {
        case NODE_STRUCT_DEF: {
            Node_struct_def* struct_def = node_unwrap_struct_def(def);
            def_base = struct_def->base;
            break;
        }
        case NODE_RAW_UNION_DEF: {
            Node_raw_union_def* raw_union_def = node_unwrap_raw_union_def(def);
            def_base = raw_union_def->base;
            break;
        }
        default:
            unreachable("");
    }

    Node_number* new_index = node_number_new(
        old_access->pos,
        get_member_index(&def_base, old_access->member_name),
        lang_type_new_from_cstr("i32", 0)
    );
    
    Llvm_load_element_ptr* new_load = llvm_load_element_ptr_new(
        old_access->pos,
        old_access->lang_type,
        0,
        load_literal(env, new_block, node_wrap_number(new_index)),
        new_callee,
        util_literal_name_new(),
        old_access->member_name,
        true
    );
    try(alloca_add(env, llvm_wrap_load_element_ptr(new_load)));

    vec_append(&a_main, &new_block->children, llvm_wrap_load_element_ptr(new_load));
    return new_load->name_self;
}

static Str_view load_ptr_index_typed(
    Env* env,
    Llvm_block* new_block,
    Node_index_typed* old_index
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
    try(alloca_add(env, llvm_wrap_load_element_ptr(new_load)));

    vec_append(&a_main, &new_block->children, llvm_wrap_load_element_ptr(new_load));
    return new_load->name_self;
}

static Str_view load_member_access_typed(
    Env* env,
    Llvm_block* new_block,
    Node_member_access_typed* old_access
) {
    Str_view ptr = load_ptr_member_access_typed(env, new_block, old_access);

    Llvm_load_another_llvm* new_load = llvm_load_another_llvm_new(
        old_access->pos,
        ptr,
        0,
        get_lang_type_from_name(env, ptr),
        util_literal_name_new()
    );
    try(alloca_add(env, llvm_wrap_load_another_llvm(new_load)));

    vec_append(&a_main, &new_block->children, llvm_wrap_load_another_llvm(new_load));
    return new_load->name;
}

static Str_view load_index_typed(
    Env* env,
    Llvm_block* new_block,
    Node_index_typed* old_index
) {
    Str_view ptr = load_ptr_index_typed(env, new_block, old_index);

    Llvm_load_another_llvm* new_load = llvm_load_another_llvm_new(
        old_index->pos,
        ptr,
        0,
        get_lang_type_from_name(env, ptr),
        util_literal_name_new()
    );
    try(alloca_add(env, llvm_wrap_load_another_llvm(new_load)));

    vec_append(&a_main, &new_block->children, llvm_wrap_load_another_llvm(new_load));
    return new_load->name;
}

// TODO: consider struct_literal to be a literal type
static Str_view load_struct_literal(
    Env* env,
    Llvm_block* new_block,
    Node_struct_literal* old_lit
) {
    (void) new_block;

    Node_struct_lit_def* new_def = node_struct_lit_def_new(
        old_lit->pos,
        old_lit->members,
        old_lit->name,
        old_lit->lang_type
    );

    try(sym_tbl_add(&env->global_literals, node_wrap_literal_def(node_wrap_struct_lit_def(new_def))));

    Llvm_struct_literal* new_lit = node_clone_struct_literal(old_lit);
    try(alloca_add(env, llvm_wrap_expr(llvm_wrap_struct_literal(new_lit))));
    return new_lit->name;
}

static Str_view load_expr(Env* env, Llvm_block* new_block, Node_expr* old_expr) {
    switch (old_expr->type) {
        case NODE_FUNCTION_CALL:
            return load_function_call(env, new_block, node_unwrap_function_call(old_expr));
        case NODE_LITERAL:
            return load_literal(env, new_block, node_unwrap_literal(old_expr));
        case NODE_SYMBOL_TYPED:
            return load_symbol_typed(env, new_block, node_unwrap_symbol_typed(old_expr));
        case NODE_SYMBOL_UNTYPED:
            unreachable("NODE_SYMBOL_UNTYPED should not still be present at this point");
        case NODE_OPERATOR:
            return load_operator(env, new_block, node_unwrap_operator(old_expr));
        case NODE_MEMBER_ACCESS_TYPED:
            return load_member_access_typed(env, new_block, node_unwrap_member_access_typed(old_expr));
        case NODE_INDEX_TYPED:
            return load_index_typed(env, new_block, node_unwrap_index_typed(old_expr));
        case NODE_STRUCT_LITERAL:
            return load_struct_literal(env, new_block, node_unwrap_struct_literal(old_expr));
        default:
            unreachable(NODE_FMT"\n", node_print(node_wrap_expr(old_expr)));
    }
}

static Llvm_reg load_function_parameters(
    Env* env,
    Llvm_block* new_fun_body,
    Node_function_params* old_params
) {
    Llvm_function_params* new_params = do_function_def_alloca(env, new_fun_body, old_params);

    for (size_t idx = 0; idx < new_params->params.info.count; idx++) {
        assert(env->ancesters.info.count > 0);
        Llvm_variable_def* param = vec_at(&new_params->params, idx);

        Llvm* dummy = NULL;
        //symbol_log(LOG_DEBUG, env);
        //alloca_log(LOG_DEBUG, env);

        bool is_struct = false;

        if (lang_type_is_struct(env, param->lang_type)) {
            is_struct = true;
        } else if (lang_type_is_enum(env, param->lang_type)) {
            is_struct = false;
        } else if (lang_type_is_primitive(env, param->lang_type)) {
            is_struct = false;
        } else if (lang_type_is_raw_union(env, param->lang_type)) {
            is_struct = true;
        } else {
            unreachable(NODE_FMT"\n", llvm_variable_def_print(param));
        }


        if (!is_struct) {
            try(alloca_add(env, llvm_wrap_def(llvm_wrap_variable_def(param))));

            Llvm_store_another_llvm* new_store = llvm_store_another_llvm_new(
                param->pos,
                param->name_self,
                param->name_corr_param,
                0,
                param->lang_type,
                util_literal_name_new()
            );
            try(alloca_add(env, llvm_wrap_store_another_llvm(new_store)));

            vec_append(&a_main, &new_fun_body->children, llvm_wrap_store_another_llvm(new_store));
        }

        log(LOG_DEBUG, NODE_FMT"\n", llvm_variable_def_print(param));
        try(alloca_lookup(&dummy, env, param->name_corr_param));

        // TODO: append to new function body instead of old
    }

    return (Llvm_reg) {
        .lang_type = {0},
        .llvm = llvm_wrap_function_params(new_params)
    };
}

static Str_view load_function_def(
    Env* env,
    Llvm_block* new_block,
    Node_function_def* old_fun_def
) {
    Pos pos = old_fun_def->pos;

    Llvm_function_decl* new_decl = llvm_function_decl_new(
        pos,
        NULL,
        node_clone_lang_type(old_fun_def->decl->return_type),
        old_fun_def->decl->name
    );

    Llvm_function_def* new_fun_def = llvm_function_def_new(
        pos,
        new_decl,
        llvm_block_new(
            pos,
            false,
            (Llvm_vec) {0},
            old_fun_def->body->symbol_collection,
            old_fun_def->body->pos_end
        ),
        0
    );

    vec_append(&a_main, &env->ancesters, &new_fun_def->body->symbol_collection);
    new_fun_def->decl->params = llvm_unwrap_function_params(load_function_parameters(
            env, new_fun_def->body, old_fun_def->decl->params
    ).llvm);
    for (size_t idx = 0; idx < old_fun_def->body->children.info.count; idx++) {
        load_node(env, new_fun_def->body, vec_at(&old_fun_def->body->children, idx));
    }
    vec_rem_last(&env->ancesters);

    vec_append(&a_main, &new_block->children, llvm_wrap_def(llvm_wrap_function_def(new_fun_def)));
    return (Str_view) {0};
}

static Str_view load_function_decl(
    Env* env,
    Llvm_block* new_block,
    Node_function_decl* old_fun_decl
) {
    (void) env;

    Llvm_function_decl* new_fun_decl = node_clone_function_decl(old_fun_decl);

    vec_append(&a_main, &new_block->children, llvm_wrap_def(llvm_wrap_function_decl(new_fun_decl)));
    return (Str_view) {0};
}

static Str_view load_return(
    Env* env,
    Llvm_block* new_block,
    Node_return* old_return
) {
    Pos pos = old_return->pos;

    Str_view result = load_expr(env, new_block, old_return->child);

    Llvm_return* new_return = llvm_return_new(
        pos,
        result,
        old_return->is_auto_inserted
    );

    vec_append(&a_main, &new_block->children, llvm_wrap_return(new_return));

    return (Str_view) {0};
}

static Str_view load_assignment(
    Env* env,
    Llvm_block* new_block,
    Node_assignment* old_assignment
) {
    assert(old_assignment->lhs);
    assert(old_assignment->rhs);

    Pos pos = old_assignment->pos;

    Llvm_store_another_llvm* new_store = llvm_store_another_llvm_new(
        pos,
        load_expr(env, new_block, old_assignment->rhs),
        load_ptr(env, new_block, old_assignment->lhs),
        0,
        node_get_lang_type(old_assignment->lhs),
        util_literal_name_new()
    );
    try(alloca_add(env, llvm_wrap_store_another_llvm(new_store)));

    assert(new_store->llvm_src.count > 0);
    assert(new_store->llvm_dest.count > 0);
    assert(old_assignment->rhs);

    vec_append(&a_main, &new_block->children, llvm_wrap_store_another_llvm(new_store));

    return new_store->name;
}

static Str_view load_variable_def(
    Env* env,
    Llvm_block* new_block,
    Node_variable_def* old_var_def
) {
    Llvm_variable_def* new_var_def = node_clone_variable_def(old_var_def);

    Llvm* alloca = NULL;
    if (!alloca_lookup(&alloca, env, new_var_def->name_self)) {
        alloca = llvm_wrap_alloca(add_load_and_store_alloca_new(env, new_var_def));
        vec_insert(&a_main, &new_block->children, 0, alloca);
    }

    vec_append(&a_main, &new_block->children, llvm_wrap_def(llvm_wrap_variable_def(new_var_def)));

    assert(alloca);
    return llvm_get_node_name(alloca);
}

static Str_view load_struct_def(
    Env* env,
    Llvm_block* new_block,
    Node_struct_def* old_struct_def
) {
    (void) env;

    vec_append(&a_main, &new_block->children, llvm_wrap_def(
        llvm_wrap_struct_def(node_clone_struct_def(old_struct_def))
    ));

    return (Str_view) {0};
}

static Str_view load_enum_def(
    Env* env,
    Llvm_block* new_block,
    Node_enum_def* old_enum_def
) {
    (void) env;

    vec_append(&a_main, &new_block->children, llvm_wrap_def(
        llvm_wrap_enum_def(node_clone_enum_def(old_enum_def))
    ));

    return (Str_view) {0};
}

static Llvm_block* if_statement_to_branch(Env* env, Node_if* if_statement, Str_view next_if, Str_view after_chain) {
    Node_block* old_block = if_statement->body;
    Llvm_block* inner_block = load_block(env, old_block);
    Llvm_block* new_block = llvm_block_new(
        old_block->pos,
        false,
        (Llvm_vec) {0},
        inner_block->symbol_collection,
        inner_block->pos_end
    );


    vec_append(&a_main, &env->ancesters, &new_block->symbol_collection);

    Node_condition* if_cond = if_statement->condition;

    Node_operator* old_oper = if_cond->child;

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
    vec_append(&a_main, &new_block->children, llvm_wrap_goto(jmp_to_after_chain));

    vec_rem_last(&env->ancesters);
    return new_block;
}

static Llvm_block* if_else_chain_to_branch(Env* env, Node_if_else_chain* if_else) {
    Llvm_block* new_block = llvm_block_new(
        if_else->pos,
        false,
        (Llvm_vec) {0},
        (Symbol_collection) {0},
        (Pos) {0}
    );

    Str_view if_after = util_literal_name_new_prefix("if_after");
    
    Llvm* dummy = NULL;
    Node_def* dummy_def = NULL;

    Str_view next_if = {0};
    for (size_t idx = 0; idx < if_else->nodes.info.count; idx++) {
        if (idx + 1 == if_else->nodes.info.count) {
            next_if = if_after;
        } else {
            next_if = util_literal_name_new_prefix("next_if");
        }

        Llvm_block* if_block = if_statement_to_branch(env, vec_at(&if_else->nodes, idx), next_if, if_after);
        vec_append(&a_main, &new_block->children, llvm_wrap_block(if_block));

        if (idx + 1 < if_else->nodes.info.count) {
            assert(!alloca_lookup(&dummy, env, next_if));
            add_label(env, new_block, next_if, vec_at(&if_else->nodes, idx)->pos, false);
            assert(alloca_lookup(&dummy, env, next_if));
        } else {
            assert(str_view_is_equal(next_if, if_after));
        }
    }

    assert(!symbol_lookup(&dummy_def, env, next_if));
    add_label(env, new_block, if_after, if_else->pos, false);
    assert(alloca_lookup(&dummy, env, next_if));
    //log_tree(LOG_DEBUG, node_wrap_block(new_block));

    return new_block;
}

static Str_view load_if_else_chain(
    Env* env,
    Llvm_block* new_block,
    Node_if_else_chain* old_if_else
) {
    Llvm_block* new_if_else = if_else_chain_to_branch(env, old_if_else);
    vec_append(&a_main, &new_block->children, llvm_wrap_block(new_if_else));

    return (Str_view) {0};
}

static Llvm_block* for_range_to_branch(Env* env, Node_for_range* old_for) {
    Str_view old_if_break = env->label_if_break;
    Str_view old_if_continue = env->label_if_continue;

    size_t init_count_ancesters = env->ancesters.info.count;

    Pos pos = old_for->pos;

    Llvm_block* new_branch_block = llvm_block_new(
        pos,
        false,
        (Llvm_vec) {0},
        old_for->body->symbol_collection,
        old_for->body->pos_end
    );

    for (size_t idx = 0; idx < old_for->body->children.info.count; idx++) {
        log(LOG_DEBUG, NODE_FMT"\n", node_print(vec_at(&old_for->body->children, idx)));
    }
    for (size_t idx = 0; idx < new_branch_block->children.info.count; idx++) {
        log(LOG_DEBUG, NODE_FMT"\n", llvm_print(vec_at(&new_branch_block->children, idx)));
    }

    vec_append(&a_main, &env->ancesters, &new_branch_block->symbol_collection);

    (void) pos;

    //vec_append(&a_main, &env->ancesters, node_wrap_block(for_block));
    Node_expr* lhs_actual = old_for->lower_bound->child;
    Node_expr* rhs_actual = old_for->upper_bound->child;

    Node_symbol_untyped* symbol_lhs_assign;
    Node_variable_def* for_var_def;
    {
        for_var_def = old_for->var_def;
        symbol_lhs_assign = node_symbol_untyped_new(for_var_def->pos, for_var_def->name);
    }

    //try(symbol_add(env, node_wrap_variable_def(for_var_def)));
    Llvm* dummy = NULL;
    Node_def* dummy_def = NULL;

    assert(symbol_lookup(&dummy_def, env, for_var_def->name));

    Node_assignment* assignment_to_inc_cond_var = for_loop_cond_var_assign_new(
        env, for_var_def->name, node_get_pos_expr(lhs_actual)
    );

    Node_operator* operator = util_binary_typed_new(
        env,
        node_wrap_symbol_untyped(node_symbol_untyped_new(symbol_lhs_assign->pos, symbol_lhs_assign->name)),
        rhs_actual,
        TOKEN_LESS_THAN
    );

    // initial assignment

    Str_view check_cond_label = util_literal_name_new_prefix("check_cond");
    Str_view assign_label = util_literal_name_new_prefix("assign_for");
    Llvm_goto* jmp_to_check_cond_label = llvm_goto_new(old_for->pos, check_cond_label, 0);
    Llvm_goto* jmp_to_assign = llvm_goto_new(old_for->pos, assign_label, 0);
    Str_view after_check_label = util_literal_name_new_prefix("after_check");
    Str_view after_for_loop_label = util_literal_name_new_prefix("after_for");

    env->label_if_break = after_for_loop_label;
    env->label_if_continue = assign_label;

    //vec_append(&a_main, &new_branch_block->children, node_wrap_variable_def(for_var_def));

    Node_assignment* new_var_assign = util_assignment_new(env, node_wrap_expr(node_wrap_symbol_untyped(symbol_lhs_assign)), lhs_actual);

    load_variable_def(env, new_branch_block, for_var_def);
    load_assignment(env, new_branch_block, new_var_assign);
    
    //log_tree(LOG_DEBUG, (Node*)new_var_assign);

    vec_append(&a_main, &new_branch_block->children, llvm_wrap_goto(jmp_to_check_cond_label));

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
                log(LOG_DEBUG, NODE_FMT"\n", llvm_print(vec_at(&new_branch_block->children, idx)));
            }
            load_node(env, new_branch_block, vec_at(&old_for->body->children, idx));
        }
    }
    assert(alloca_lookup(&dummy, env, check_cond_label));

    vec_append(&a_main, &new_branch_block->children, llvm_wrap_goto(jmp_to_assign));
    {
        vec_rem_last(&env->ancesters);
        add_label(env, new_branch_block, assign_label, pos, false);
        vec_append(&a_main, &env->ancesters, &new_branch_block->symbol_collection);
    }
    load_assignment(env, new_branch_block, assignment_to_inc_cond_var);
    vec_append(&a_main, &new_branch_block->children, llvm_wrap_goto(llvm_goto_new(pos, check_cond_label, 0)));
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
    Node_for_range* old_for
) {
    Llvm_block* new_for = for_range_to_branch(env, old_for);
    vec_append(&a_main, &new_block->children, llvm_wrap_block(new_for));

    return (Str_view) {0};
}

static Llvm_block* for_with_cond_to_branch(Env* env, Node_for_with_cond* old_for) {
    Str_view old_if_break = env->label_if_break;
    Str_view old_if_continue = env->label_if_continue;

    Pos pos = old_for->pos;

    Llvm_block* new_branch_block = llvm_block_new(
        pos,
        false,
        (Llvm_vec) {0},
        old_for->body->symbol_collection,
        old_for->body->pos_end
    );

    vec_append(&a_main, &env->ancesters, &new_branch_block->symbol_collection);


    Node_operator* operator = old_for->condition->child;
    Str_view check_cond_label = util_literal_name_new_prefix("check_cond");
    Llvm_goto* jmp_to_check_cond_label = llvm_goto_new(old_for->pos, check_cond_label, 0);
    Str_view after_check_label = util_literal_name_new_prefix("for_body");
    Str_view after_for_loop_label = util_literal_name_new_prefix("after_for_loop");

    env->label_if_break = after_for_loop_label;
    env->label_if_continue = check_cond_label;

    vec_append(&a_main, &new_branch_block->children, llvm_wrap_goto(jmp_to_check_cond_label));

    add_label(env, new_branch_block, check_cond_label, pos, true);

    //load_operator(env, new_branch_block, operator);
    //Node* dummy = NULL;
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
    //    log(LOG_DEBUG, NODE_FMT"\n", node_print(vec_at(&new_branch_block->children, idx)));
    //}
    log(LOG_DEBUG, "DSFJKLKJDFS: "STR_VIEW_FMT"\n", str_view_print(after_check_label));


    //try(symbol_lookup(&dummy, env, str_view_from_cstr("str18")));
    //for (size_t idx = 0; idx < old_for->body->children.info.count; idx++) {
    //    log(LOG_DEBUG, NODE_FMT"\n", node_print(vec_at(&old_for->body->children, idx)));
    //}

    for (size_t idx = 0; idx < old_for->body->children.info.count; idx++) {
        //for (size_t idx = 0; idx < new_branch_block->children.info.count; idx++) {
        //    log(LOG_DEBUG, NODE_FMT"\n", node_print(vec_at(&new_branch_block->children, idx)));
        //}
        load_node(env, new_branch_block, vec_at(&old_for->body->children, idx));
    }

    vec_append(&a_main, &new_branch_block->children, llvm_wrap_goto(
        llvm_goto_new(old_for->pos, check_cond_label, 0)
    ));
    add_label(env, new_branch_block, after_for_loop_label, pos, true);

    Llvm* dummy = NULL;

    env->label_if_continue = old_if_continue;
    env->label_if_break = old_if_break;
    for (size_t idx = 0; idx < env->defered_allocas_to_add.info.count; idx++) {
        log(LOG_DEBUG, NODE_FMT, llvm_print(vec_at(&env->defered_allocas_to_add, idx)));
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
    Node_for_with_cond* old_for
) {
    Llvm_block* new_for = for_with_cond_to_branch(env, old_for);
    vec_append(&a_main, &new_block->children, llvm_wrap_block(new_for));

    return (Str_view) {0};
}

static Str_view load_break(
    Env* env,
    Llvm_block* new_block,
    Node_break* old_break
) {
    // TODO: make expected failure test case for break in invalid location
    if (env->label_if_break.count < 1) {
        unreachable("cannot break here\n");
    }

    Llvm_goto* new_goto = llvm_goto_new(old_break->pos, env->label_if_break, 0);
    vec_append(&a_main, &new_block->children, llvm_wrap_goto(new_goto));

    return (Str_view) {0};
}

static Str_view load_continue(
    Env* env,
    Llvm_block* new_block,
    Node_continue* old_continue
) {
    // TODO: make expected failure test case for continue in invalid location
    if (env->label_if_continue.count < 1) {
        unreachable("cannot continue here\n");
    }

    Llvm_goto* new_goto = llvm_goto_new(old_continue->pos, env->label_if_continue, 0);
    vec_append(&a_main, &new_block->children, llvm_wrap_goto(new_goto));

    return (Str_view) {0};
}

static Str_view load_raw_union_def(
    Env* env,
    Llvm_block* new_block,
    Node_raw_union_def* old_def
) {
    (void) env;

    vec_append(&a_main, &new_block->children, llvm_wrap_def(
        llvm_wrap_raw_union_def(node_clone_raw_union_def(old_def))
    ));

    return (Str_view) {0};
}

static Str_view load_ptr_variable_def(
    Env* env,
    Llvm_block* new_block,
    Node_variable_def* old_variable_def
) {
    return load_variable_def(env, new_block, old_variable_def);
}

static Str_view load_ptr_unary(
    Env* env,
    Llvm_block* new_block,
    Node_unary* old_unary
) {
    Pos pos = old_unary->pos;

    switch (old_unary->token_type) {
        case TOKEN_DEREF: {
            Lang_type lang_type = node_get_lang_type_expr(old_unary->child);

            if (lang_type_is_struct(env, lang_type)) {
                return load_ptr_expr(env, new_block, old_unary->child);
            } else if (lang_type_is_enum(env, lang_type)) {
            } else if (lang_type_is_primitive(env, lang_type)) {
            } else if (lang_type_is_raw_union(env, lang_type)) {
                return load_ptr_expr(env, new_block, old_unary->child);
            } else {
                unreachable("");
            }
            
            Str_view ptr = load_ptr_expr(env, new_block, old_unary->child);
            Llvm_load_another_llvm* new_load = llvm_load_another_llvm_new(
                pos,
                ptr,
                0,
                old_unary->lang_type,
                util_literal_name_new()
            );
            try(alloca_add(env, llvm_wrap_load_another_llvm(new_load)));
            new_load->lang_type.pointer_depth++;

            vec_append(&a_main, &new_block->children, llvm_wrap_load_another_llvm(new_load));
            return new_load->name;
        }
        default:
            todo();
    }
    todo();
}

static Str_view load_ptr_operator(Env* env, Llvm_block* new_block, Node_operator* old_oper) {
    switch (old_oper->type) {
        case NODE_BINARY:
            todo();
        case NODE_UNARY:
            return load_ptr_unary(env, new_block, node_unwrap_unary(old_oper));
        default:
            unreachable("");
    }
}

static Str_view load_ptr_expr(Env* env, Llvm_block* new_block, Node_expr* old_expr) {
    switch (old_expr->type) {
        case NODE_SYMBOL_TYPED:
            return load_ptr_symbol_typed(env, new_block, node_unwrap_symbol_typed(old_expr));
        case NODE_MEMBER_ACCESS_TYPED:
            return load_ptr_member_access_typed(env, new_block, node_unwrap_member_access_typed(old_expr));
        case NODE_OPERATOR:
            return load_ptr_operator(env, new_block, node_unwrap_operator(old_expr));
        case NODE_INDEX_TYPED:
            return load_ptr_index_typed(env, new_block, node_unwrap_index_typed(old_expr));
        default:
            unreachable(NODE_FMT"\n", node_print(node_wrap_expr(old_expr)));
    }
}

static Str_view load_ptr_def(Env* env, Llvm_block* new_block, Node_def* old_def) {
    switch (old_def->type) {
        case NODE_VARIABLE_DEF:
            return load_ptr_variable_def(env, new_block, node_unwrap_variable_def(old_def));
        default:
            unreachable("");
    }
}

static Str_view load_ptr(Env* env, Llvm_block* new_block, Node* old_node) {
    switch (old_node->type) {
        case NODE_EXPR:
            return load_ptr_expr(env, new_block, node_unwrap_expr(old_node));
        case NODE_DEF:
            return load_ptr_def(env, new_block, node_unwrap_def(old_node));
        default:
            unreachable(NODE_FMT"\n", node_print(old_node));
    }
}

static Str_view load_def(Env* env, Llvm_block* new_block, Node_def* old_def) {
    switch (old_def->type) {
        case NODE_FUNCTION_DEF:
            return load_function_def(env, new_block, node_unwrap_function_def(old_def));
        case NODE_FUNCTION_DECL:
            return load_function_decl(env, new_block, node_unwrap_function_decl(old_def));
        case NODE_VARIABLE_DEF:
            return load_variable_def(env, new_block, node_unwrap_variable_def(old_def));
        case NODE_STRUCT_DEF:
            return load_struct_def(env, new_block, node_unwrap_struct_def(old_def));
        case NODE_ENUM_DEF:
            return load_enum_def(env, new_block, node_unwrap_enum_def(old_def));
        case NODE_RAW_UNION_DEF:
            return load_raw_union_def(env, new_block, node_unwrap_raw_union_def(old_def));
        case NODE_LITERAL_DEF:
            unreachable("");
        case NODE_PRIMITIVE_DEF:
            unreachable("");
    }
    unreachable("");
}

static Str_view load_node(Env* env, Llvm_block* new_block, Node* old_node) {
    switch (old_node->type) {
        case NODE_EXPR:
            return load_expr(env, new_block, node_unwrap_expr(old_node));
        case NODE_DEF:
            return load_def(env, new_block, node_unwrap_def(old_node));
        case NODE_RETURN:
            return load_return(env, new_block, node_unwrap_return(old_node));
        case NODE_ASSIGNMENT:
            return load_assignment(env, new_block, node_unwrap_assignment(old_node));
        case NODE_IF_ELSE_CHAIN:
            return load_if_else_chain(env, new_block, node_unwrap_if_else_chain(old_node));
        case NODE_FOR_RANGE:
            return load_for_range(env, new_block, node_unwrap_for_range(old_node));
        case NODE_FOR_WITH_COND:
            return load_for_with_cond(env, new_block, node_unwrap_for_with_cond(old_node));
        case NODE_BREAK:
            return load_break(env, new_block, node_unwrap_break(old_node));
        case NODE_CONTINUE:
            return load_continue(env, new_block, node_unwrap_continue(old_node));
        case NODE_BLOCK:
            vec_append(
                &a_main,
                &new_block->children,
                llvm_wrap_block(load_block(env, node_unwrap_block(old_node)))
            );
            return (Str_view) {0};
        default:
            unreachable(NODE_FMT"\n", node_print(old_node));
    }
}

// TODO: rethink how to do block, because this is simply not working
static Llvm_block* load_block(Env* env, Node_block* old_block) {
    size_t init_count_ancesters = env->ancesters.info.count;

    Llvm_block* new_block = llvm_block_new(
        old_block->pos,
        old_block->is_variadic,
        (Llvm_vec) {0},
        old_block->symbol_collection,
        old_block->pos_end
    );

    vec_append(&a_main, &env->ancesters, &new_block->symbol_collection);

    for (size_t idx = 0; idx < old_block->children.info.count; idx++) {
        load_node(env, new_block, vec_at(&old_block->children, idx));
    }

    vec_rem_last(&env->ancesters);

    try(init_count_ancesters == env->ancesters.info.count);
    return new_block;
}

Llvm_block* add_load_and_store(Env* env, Node_block* old_root) {
    Llvm_block* block = load_block(env, old_root);
    return block;
}

