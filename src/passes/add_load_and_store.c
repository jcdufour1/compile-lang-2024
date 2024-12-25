#include <node.h>
#include <llvm.h>
#include <nodes.h>
#include <symbol_table.h>
#include <parser_utils.h>

#include "passes.h"

static Llvm_reg llvm_expr_reg_wrap(Llvm_expr_reg old_reg) {
    return (Llvm_reg) {
        .lang_type = old_reg.lang_type,
        .llvm = llvm_wrap_expr(old_reg.llvm)
    };
}

static Llvm_variable_def* node_clone_variable_def(Node_variable_def* old_var_def);

static Llvm_alloca* add_load_and_store_alloca_new(Env* env, Llvm_variable_def* var_def) {
    Llvm_alloca* alloca = llvm_alloca_new(var_def->pos);
    alloca->name = var_def->name;
    alloca->lang_type = var_def->lang_type;
    alloca_add(env, llvm_wrap_alloca(alloca));
    assert(alloca);
    return alloca;
}

static Llvm_function_params* node_clone_function_params(Node_function_params* old_params) {
    Llvm_function_params* new_params = llvm_function_params_new(old_params->pos);

    for (size_t idx = 0; idx < old_params->params.info.count; idx++) {
        vec_append(&a_main, &new_params->params, node_clone_variable_def(vec_at(&old_params->params, idx)));
    }

    return new_params;
}

static Llvm_lang_type* node_clone_lang_type(Node_lang_type* old_lang_type) {
    Llvm_lang_type* new_lang_type = llvm_lang_type_new(old_lang_type->pos);
    new_lang_type->lang_type = old_lang_type->lang_type;
    return new_lang_type;
}

static Llvm_function_decl* node_clone_function_decl(Node_function_decl* old_decl) {
    Llvm_function_decl* new_decl = llvm_function_decl_new(old_decl->pos);
    new_decl->params = node_clone_function_params(old_decl->params);
    new_decl->return_type = node_clone_lang_type(old_decl->return_type);
    new_decl->name = old_decl->name;
    return new_decl;
}

static Llvm_variable_def* node_clone_variable_def(Node_variable_def* old_var_def) {
    Llvm_variable_def* new_var_def = llvm_variable_def_new(old_var_def->pos);
    new_var_def->lang_type = old_var_def->lang_type;
    new_var_def->is_variadic = old_var_def->is_variadic;
    new_var_def->name = old_var_def->name;
    return new_var_def;
}

static Llvm_struct_literal* node_clone_struct_literal(const Node_struct_literal* old_lit) {
    Llvm_struct_literal* new_lit = llvm_struct_literal_new(old_lit->pos);
    new_lit->members = old_lit->members;
    new_lit->name = old_lit->name;
    new_lit->lang_type = old_lit->lang_type;

    return new_lit;
}

static Llvm_struct_def* node_clone_struct_def(const Node_struct_def* old_def) {
    Llvm_struct_def* new_def = llvm_struct_def_new(old_def->pos);
    new_def->base = old_def->base;
    return new_def;
}

static Llvm_enum_def* node_clone_enum_def(const Node_enum_def* old_def) {
    Llvm_enum_def* new_def = llvm_enum_def_new(old_def->pos);
    new_def->base = old_def->base;
    return new_def;
}

static Llvm_raw_union_def* node_clone_raw_union_def(const Node_raw_union_def* old_def) {
    Llvm_raw_union_def* new_def = llvm_raw_union_def_new(old_def->pos);
    new_def->base = old_def->base;
    return new_def;
}

static Llvm_function_params* do_function_def_alloca(Env* env, Llvm_block* new_block, const Node_function_params* old_params) {
    Llvm_function_params* new_params = llvm_function_params_new(old_params->pos);

    Llvm* dummy = NULL;

    for (size_t idx = 0; idx < old_params->params.info.count; idx++) {
        Llvm_variable_def* param = node_clone_variable_def(vec_at(&old_params->params, idx));

        if (lang_type_is_struct(env, param->lang_type)) {
            alloca_add(env, llvm_wrap_def(llvm_wrap_variable_def(param)));
        } else if (lang_type_is_enum(env, param->lang_type)) {
            vec_insert(&a_main, &new_block->children, 0, llvm_wrap_alloca(
                add_load_and_store_alloca_new(env, param)
            ));
        } else if (lang_type_is_raw_union(env, param->lang_type)) {
            alloca_add(env, llvm_wrap_def(llvm_wrap_variable_def(param)));
        } else if (lang_type_is_primitive(env, param->lang_type)) {
            vec_insert(&a_main, &new_block->children, 0, llvm_wrap_alloca(
                add_load_and_store_alloca_new(env, param)
            ));
        } else {
            todo();
        }

        vec_append(&a_main, &new_params->params, param);

        assert(alloca_lookup(&dummy, env, param->name));
    }

    return new_params;
}

static Llvm_block* load_block(Env* env, Node_block* old_block);

static Llvm_expr_reg load_expr(Env* env, Llvm_block* new_block, Node_expr* old_expr);

static Llvm_reg load_ptr(Env* env, Llvm_block* new_block, Node* old_node);

static Llvm_reg load_ptr_expr(Env* env, Llvm_block* new_block, Node_expr* old_expr);

static Llvm_reg load_node(Env* env, Llvm_block* new_block, Node* old_node);

static Llvm_reg load_operator(
    Env* env,
    Llvm_block* new_block,
    Node_operator* old_oper
);

static Llvm_goto* goto_new(Str_view name_label_to_jmp_to, Pos pos) {
    Llvm_goto* lang_goto = llvm_goto_new(pos);
    lang_goto->name = name_label_to_jmp_to;
    return lang_goto;
}

static void add_label(Env* env, Llvm_block* block, Str_view label_name, Pos pos, bool defer_add_sym) {
    Llvm_label* label = llvm_label_new(pos);
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

    Llvm_cond_goto* cond_goto = llvm_cond_goto_new(pos);
    cond_goto->llvm_src = load_operator(env, block, old_oper);
    cond_goto->if_true = label_name_if_true;
    cond_goto->if_false = label_name_if_false;

    vec_append(&a_main, &block->children, llvm_wrap_cond_goto(cond_goto));
}

static Node_assignment* for_loop_cond_var_assign_new(Env* env, Str_view sym_name, Pos pos) {
    Node_literal* literal = util_literal_new_from_int64_t(1, TOKEN_INT_LITERAL, pos);
    Node_operator* operator = util_binary_typed_new(
        env,
        node_wrap_symbol_untyped(util_symbol_new(sym_name, pos)),
        node_wrap_literal(literal),
        TOKEN_SINGLE_PLUS
    );
    return util_assignment_new(
        env,
        node_wrap_expr(node_wrap_symbol_untyped(util_symbol_new(sym_name, pos))),
        node_wrap_operator(operator)
    );
}

static Llvm_reg load_function_call(
    Env* env,
    Llvm_block* new_block,
    Node_function_call* old_fun_call
) {

    Llvm_function_call* new_fun_call = llvm_function_call_new(old_fun_call->pos);

    for (size_t idx = 0; idx < old_fun_call->args.info.count; idx++) {
        Node_expr* old_arg = vec_at(&old_fun_call->args, idx);
        Pos arg_pos = node_get_pos_expr(old_arg);

        Llvm_llvm_placeholder* new_arg = llvm_llvm_placeholder_new(arg_pos);
        new_arg->lang_type = node_get_lang_type_expr(old_arg);
        new_arg->llvm_reg = llvm_expr_reg_wrap(load_expr(env, new_block, old_arg));
        vec_append(&a_main, &new_fun_call->args, llvm_wrap_llvm_placeholder(new_arg));
    }

    new_fun_call->name = old_fun_call->name;
    new_fun_call->lang_type = old_fun_call->lang_type;
    
    vec_append(&a_main, &new_block->children, llvm_wrap_expr(llvm_wrap_function_call(new_fun_call)));
    return (Llvm_reg) {
        .lang_type = old_fun_call->lang_type,
        .llvm = llvm_wrap_expr(llvm_wrap_function_call(new_fun_call))
    };
}

static Llvm_literal* node_literal_clone(Node_literal* old_lit) {
    Llvm_literal* new_lit = NULL;

    switch (old_lit->type) {
        case NODE_STRING: {
            Llvm_string* string = llvm_string_new(node_get_pos_literal(old_lit));
            string->data = node_unwrap_string(old_lit)->data;
            string->lang_type = node_unwrap_string(old_lit)->lang_type;
            string->name = node_unwrap_string(old_lit)->name;
            new_lit = llvm_wrap_string(string);
            break;
        }
        case NODE_VOID: {
            Llvm_void* lang_void = llvm_void_new(node_get_pos_literal(old_lit));
            new_lit = llvm_wrap_void(lang_void);
            break;
        }
        case NODE_ENUM_LIT: {
            Llvm_enum_lit* enum_lit = llvm_enum_lit_new(node_get_pos_literal(old_lit));
            enum_lit->data = node_unwrap_enum_lit(old_lit)->data;
            enum_lit->lang_type = node_unwrap_enum_lit(old_lit)->lang_type;
            new_lit = llvm_wrap_enum_lit(enum_lit);
            break;
        }
        case NODE_CHAR: {
            Llvm_char* lang_char = llvm_char_new(node_get_pos_literal(old_lit));
            lang_char->data = node_unwrap_char(old_lit)->data;
            lang_char->lang_type = node_unwrap_char(old_lit)->lang_type;
            new_lit = llvm_wrap_char(lang_char);
            break;
        }
        case NODE_NUMBER: {
            Llvm_number* number = llvm_number_new(node_get_pos_literal(old_lit));
            number->data = node_unwrap_number(old_lit)->data;
            number->lang_type = node_unwrap_number(old_lit)->lang_type;
            new_lit = llvm_wrap_number(number);
            break;
        }
        default:
            unreachable(NODE_FMT"\n", node_print((Node*)old_lit));
    }

    return new_lit;
}

static Llvm_literal_reg load_literal(
    Env* env,
    Llvm_block* new_block,
    Node_literal* old_lit
) {
    (void) env;
    (void) new_block;
    Pos pos = node_get_pos_literal(old_lit);

    Llvm_literal* new_lit = node_literal_clone(old_lit);

    if (new_lit->type == LLVM_STRING) {
        Node_string_def* new_def = node_string_def_new(pos);
        new_def->data = node_unwrap_string(old_lit)->data;
        new_def->name = node_unwrap_string(old_lit)->name;
        try(sym_tbl_add(&env->global_literals, node_wrap_literal_def(node_wrap_string_def(new_def))));
    }

    return (Llvm_literal_reg) {
        .lang_type = node_get_lang_type_literal(old_lit),
        .llvm = new_lit
    };
}

static Llvm_reg load_ptr_symbol_typed(
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
    if (!alloca_lookup(&alloca, env, var_def->name)) {
        alloca = llvm_wrap_alloca(add_load_and_store_alloca_new(env, var_def));
        vec_append(&a_main, &new_block->children, alloca);
    }

    assert(var_def);
    log(LOG_DEBUG, NODE_FMT"\n", llvm_variable_def_print(var_def));
    log(LOG_DEBUG, NODE_FMT"\n", node_symbol_typed_print(old_sym));
    log(LOG_DEBUG, NODE_FMT"\n", llvm_print(alloca));
    //log(LOG_DEBUG, NODE_FMT"\n", node_print(var_def->storage_location.node));
    assert(lang_type_is_equal(var_def->lang_type, node_get_lang_type_symbol_typed(old_sym)));

    return (Llvm_reg) {
        .lang_type = llvm_get_lang_type(alloca),
        .llvm = alloca
    };
}

static Llvm_reg load_symbol_typed(
    Env* env,
    Llvm_block* new_block,
    Node_symbol_typed* old_sym
) {
    Pos pos = node_get_pos_symbol_typed(old_sym);

    Llvm_load_another_llvm* new_load = llvm_load_another_llvm_new(pos);
    new_load->llvm_src = load_ptr_symbol_typed(env, new_block, old_sym);
    new_load->lang_type = node_get_lang_type_symbol_typed(old_sym);

    vec_append(&a_main, &new_block->children, llvm_wrap_load_another_llvm(new_load));
    return (Llvm_reg) {
        .lang_type = node_get_lang_type_symbol_typed(old_sym),
        .llvm = llvm_wrap_load_another_llvm(new_load)
    };
}

static Llvm_reg load_binary(
    Env* env,
    Llvm_block* new_block,
    Node_binary* old_bin
) {
    Llvm_binary* new_bin = llvm_binary_new(old_bin->pos);

    new_bin->lhs = load_expr(env, new_block, old_bin->lhs);
    new_bin->rhs = load_expr(env, new_block, old_bin->rhs);

    new_bin->token_type = old_bin->token_type;
    new_bin->lang_type = old_bin->lang_type;

    vec_append(&a_main, &new_block->children, llvm_wrap_expr(llvm_wrap_operator(llvm_wrap_binary(new_bin))));
    return (Llvm_reg) {
        .lang_type = new_bin->lang_type,
        .llvm = llvm_wrap_expr(llvm_wrap_operator(llvm_wrap_binary(new_bin)))
    };
}

static Llvm_reg load_unary(
    Env* env,
    Llvm_block* new_block,
    Node_unary* old_unary
) {
    switch (old_unary->token_type) {
        case TOKEN_DEREF: {
            if (lang_type_is_struct(env, node_get_lang_type_expr(old_unary->child))) {
                todo();
            } else if (lang_type_is_primitive(env, node_get_lang_type_expr(old_unary->child))) {
                Llvm_expr_reg ptr = load_expr(env, new_block, old_unary->child);
                Llvm_load_another_llvm* new_load = llvm_load_another_llvm_new(old_unary->pos);
                new_load->llvm_src = llvm_expr_reg_wrap(ptr);
                new_load->lang_type = old_unary->lang_type;

                vec_append(&a_main, &new_block->children, llvm_wrap_load_another_llvm(new_load));
                return (Llvm_reg) {
                    .lang_type = old_unary->lang_type,
                    .llvm = llvm_wrap_load_another_llvm(new_load)
                };
            } else {
                unreachable("");
            }
        }
        case TOKEN_REFER: {
            return load_ptr_expr(env, new_block, old_unary->child);
        }
        case TOKEN_UNSAFE_CAST:
            if (old_unary->lang_type.pointer_depth > 0 && node_get_lang_type_expr(old_unary->child).pointer_depth > 0) {
                return llvm_expr_reg_wrap(load_expr(env, new_block, old_unary->child));
            }
            // fallthrough
        case TOKEN_NOT: {
            Llvm_unary* new_unary = llvm_unary_new(old_unary->pos);
            new_unary->child = load_expr(env, new_block, old_unary->child);

            new_unary->token_type = old_unary->token_type;
            new_unary->lang_type = old_unary->lang_type;

            vec_append(&a_main, &new_block->children, llvm_wrap_expr(llvm_wrap_operator(llvm_wrap_unary(new_unary))));
            return (Llvm_reg) {
                .lang_type = new_unary->lang_type,
                .llvm = llvm_wrap_expr(llvm_wrap_operator(llvm_wrap_unary(new_unary)))
            };
        }
        default:
            unreachable(
                NODE_FMT " %d\n",
                node_print(node_wrap_expr(node_wrap_operator(node_wrap_unary(old_unary)))),
                old_unary->token_type
            );
    }
}

static Llvm_reg load_operator(
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

static Llvm_reg load_ptr_member_access_typed(
    Env* env,
    Llvm_block* new_block,
    Node_member_access_typed* old_access
) {
    Llvm_reg new_callee = load_ptr_expr(env, new_block, old_access->callee);

    Node_def* def = NULL;
    try(symbol_lookup(&def, env, llvm_get_lang_type(new_callee.llvm).str));

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

    Node_number* new_index = node_number_new(old_access->pos);
    new_index->data = get_member_index(&def_base, old_access->member_name);
    new_index->lang_type = lang_type_new_from_cstr("i32", 0);
    
    Llvm_load_element_ptr* new_load = llvm_load_element_ptr_new(old_access->pos);
    new_load->lang_type = old_access->lang_type;
    new_load->struct_index = llvm_wrap_literal_reg(load_literal(env, new_block, node_wrap_number(new_index)));
    new_load->llvm_src = new_callee;
    new_load->name = old_access->member_name;
    new_load->is_from_struct = true;

    vec_append(&a_main, &new_block->children, llvm_wrap_load_element_ptr(new_load));
    return (Llvm_reg) {
        .lang_type = new_load->lang_type,
        .llvm = llvm_wrap_load_element_ptr(new_load)
    };
    //assert(result.node);
    //return result;
}

static Llvm_reg load_ptr_index_typed(
    Env* env,
    Llvm_block* new_block,
    Node_index_typed* old_index
) {
    Llvm_load_element_ptr* new_load = llvm_load_element_ptr_new(old_index->pos);
    new_load->lang_type = old_index->lang_type;
    new_load->struct_index = load_expr(env, new_block, old_index->index);
    new_load->llvm_src = load_expr(env, new_block, old_index->callee);
    new_load->name = str_view_from_cstr("");
    new_load->is_from_struct = false;

    vec_append(&a_main, &new_block->children, llvm_wrap_load_element_ptr(new_load));
    return (Llvm_reg) {
        .lang_type = new_load->lang_type,
        .llvm = llvm_wrap_load_element_ptr(new_load)
    };
}

static Llvm_reg load_member_access_typed(
    Env* env,
    Llvm_block* new_block,
    Node_member_access_typed* old_access
) {
    Llvm_reg ptr = load_ptr_member_access_typed(env, new_block, old_access);

    Llvm_load_another_llvm* new_load = llvm_load_another_llvm_new(old_access->pos);
    new_load->llvm_src = ptr;
    new_load->lang_type = ptr.lang_type;

    vec_append(&a_main, &new_block->children, llvm_wrap_load_another_llvm(new_load));
    return (Llvm_reg) {
        .lang_type = new_load->lang_type,
        .llvm = llvm_wrap_load_another_llvm(new_load)
    };
}

static Llvm_reg load_index_typed(
    Env* env,
    Llvm_block* new_block,
    Node_index_typed* old_index
) {
    Llvm_reg ptr = load_ptr_index_typed(env, new_block, old_index);

    Llvm_load_another_llvm* new_load = llvm_load_another_llvm_new(old_index->pos);
    new_load->llvm_src = ptr;
    new_load->lang_type = ptr.lang_type;

    vec_append(&a_main, &new_block->children, llvm_wrap_load_another_llvm(new_load));
    return (Llvm_reg) {
        .lang_type = new_load->lang_type,
        .llvm = llvm_wrap_load_another_llvm(new_load)
    };
}

// TODO: consider struct_literal to be a literal type
static Llvm_reg load_struct_literal(
    Env* env,
    Llvm_block* new_block,
    Node_struct_literal* old_lit
) {
    (void) new_block;
    Node_struct_lit_def* new_def = node_struct_lit_def_new(old_lit->pos);
    new_def->members = old_lit->members;
    new_def->name = old_lit->name;
    new_def->lang_type = old_lit->lang_type;

    try(sym_tbl_add(&env->global_literals, node_wrap_literal_def(node_wrap_struct_lit_def(new_def))));

    return (Llvm_reg) {
        .lang_type = old_lit->lang_type,
        .llvm = llvm_wrap_expr(llvm_wrap_struct_literal(node_clone_struct_literal(old_lit)))
    };
}

static Llvm_reg load_expr(Env* env, Llvm_block* new_block, Node_expr* old_expr) {
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

        //symbol_log(LOG_DEBUG, env);
        //alloca_log(LOG_DEBUG, env);

        if (lang_type_is_struct(env, param->lang_type)) {
            continue;
        } else if (lang_type_is_enum(env, param->lang_type)) {
        } else if (lang_type_is_primitive(env, param->lang_type)) {
        } else if (lang_type_is_raw_union(env, param->lang_type)) {
            continue;
        } else {
            unreachable(NODE_FMT"\n", node_print((Node*)param));
        }

        Llvm_reg fun_param_call = llvm_register_sym_new(llvm_wrap_def(llvm_wrap_variable_def(param)));
        
        Llvm_store_another_llvm* new_store = llvm_store_another_llvm_new(param->pos);
        new_store->llvm_src = fun_param_call;
        new_store->llvm_dest = get_storage_location(env, param->name);
        new_store->lang_type = param->lang_type;

        // TODO: append to new function body instead of old
        vec_append(&a_main, &new_fun_body->children, llvm_wrap_store_another_llvm(new_store));
    }

    return (Llvm_reg) {
        .lang_type = {0},
        .llvm = llvm_wrap_function_params(new_params)
    };
}

static Llvm_reg load_function_def(
    Env* env,
    Llvm_block* new_block,
    Node_function_def* old_fun_def
) {
    Pos pos = old_fun_def->pos;

    Llvm_function_def* new_fun_def = llvm_function_def_new(pos);
    //new_fun_def->declaration = node_clone_function_decl(old_fun_def->declaration);
    new_fun_def->body = llvm_block_new(pos);

    new_fun_def->decl = llvm_function_decl_new(pos);
    new_fun_def->decl->return_type = node_clone_lang_type(old_fun_def->decl->return_type);
    new_fun_def->decl->name = old_fun_def->decl->name;

    new_fun_def->body->symbol_collection = old_fun_def->body->symbol_collection;
    new_fun_def->body->pos_end = old_fun_def->body->pos_end;

    vec_append(&a_main, &env->ancesters, &new_fun_def->body->symbol_collection);
    new_fun_def->decl->params = llvm_unwrap_function_params(load_function_parameters(
            env, new_fun_def->body, old_fun_def->decl->params
    ).llvm);
    for (size_t idx = 0; idx < old_fun_def->body->children.info.count; idx++) {
        load_node(env, new_fun_def->body, vec_at(&old_fun_def->body->children, idx));
    }
    vec_rem_last(&env->ancesters);

    vec_append(&a_main, &new_block->children, llvm_wrap_def(llvm_wrap_function_def(new_fun_def)));
    return (Llvm_reg) {
        .lang_type = old_fun_def->decl->return_type->lang_type,
        .llvm = llvm_wrap_def(llvm_wrap_function_def(new_fun_def))
    };
}

static Llvm_reg load_function_decl(
    Env* env,
    Llvm_block* new_block,
    Node_function_decl* old_fun_decl
) {
    (void) env;

    Llvm_function_decl* new_fun_decl = node_clone_function_decl(old_fun_decl);

    vec_append(&a_main, &new_block->children, llvm_wrap_def(llvm_wrap_function_decl(new_fun_decl)));
    return (Llvm_reg) {
        .lang_type = old_fun_decl->return_type->lang_type,
        .llvm = llvm_wrap_def(llvm_wrap_function_decl(new_fun_decl))
    };
}

static Llvm_reg load_return(
    Env* env,
    Llvm_block* new_block,
    Node_return* old_return
) {
    Pos pos = old_return->pos;

    Llvm_return* new_return = llvm_return_new(pos);
    Llvm_reg result = load_expr(env, new_block, old_return->child);
    Llvm_llvm_placeholder* new_child = llvm_llvm_placeholder_new(pos);
    new_child->llvm_reg = result;
    new_child->lang_type = result.lang_type;
    new_return->child = llvm_wrap_llvm_placeholder(new_child);

    vec_append(&a_main, &new_block->children, llvm_wrap_return(new_return));

    return (Llvm_reg) {0};
}

static Llvm_reg load_assignment(
    Env* env,
    Llvm_block* new_block,
    Node_assignment* old_assignment
) {
    assert(old_assignment->lhs);
    assert(old_assignment->rhs);

    Pos pos = old_assignment->pos;

    Llvm_store_another_llvm* new_store = llvm_store_another_llvm_new(pos);
    new_store->lang_type = node_get_lang_type(old_assignment->lhs);
    new_store->llvm_src = load_expr(env, new_block, old_assignment->rhs);
    new_store->llvm_dest = load_ptr(env, new_block, old_assignment->lhs);

    assert(new_store->llvm_src.llvm);
    assert(new_store->llvm_dest.llvm);
    assert(old_assignment->rhs);

    vec_append(&a_main, &new_block->children, llvm_wrap_store_another_llvm(new_store));

    return (Llvm_reg) {
        .lang_type = new_store->lang_type,
        .llvm = llvm_wrap_store_another_llvm(new_store)
    };
}

static Llvm_reg load_variable_def(
    Env* env,
    Llvm_block* new_block,
    Node_variable_def* old_var_def
) {
    Llvm_variable_def* new_var_def = node_clone_variable_def(old_var_def);

    Llvm* alloca = NULL;
    if (!alloca_lookup(&alloca, env, new_var_def->name)) {
        alloca = llvm_wrap_alloca(add_load_and_store_alloca_new(env, new_var_def));
        vec_insert(&a_main, &new_block->children, 0, alloca);
    }

    vec_append(&a_main, &new_block->children, llvm_wrap_def(llvm_wrap_variable_def(new_var_def)));

    assert(alloca);
    return (Llvm_reg) {
        .lang_type = llvm_get_lang_type(alloca),
        .llvm = alloca
    };
}

static Llvm_reg load_struct_def(
    Env* env,
    Llvm_block* new_block,
    Node_struct_def* old_struct_def
) {
    (void) env;

    vec_append(&a_main, &new_block->children, llvm_wrap_def(
        llvm_wrap_struct_def(node_clone_struct_def(old_struct_def))
    ));

    return (Llvm_reg) {0};
}

static Llvm_reg load_enum_def(
    Env* env,
    Llvm_block* new_block,
    Node_enum_def* old_enum_def
) {
    (void) env;

    vec_append(&a_main, &new_block->children, llvm_wrap_def(
        llvm_wrap_enum_def(node_clone_enum_def(old_enum_def))
    ));

    return (Llvm_reg) {0};
}

static Llvm_block* if_statement_to_branch(Env* env, Node_if* if_statement, Str_view next_if, Str_view after_chain) {
    Node_block* old_block = if_statement->body;
    Llvm_block* new_block = llvm_block_new(old_block->pos);

    Llvm_block* inner_block = load_block(env, old_block);
    new_block->symbol_collection = inner_block->symbol_collection;
    new_block->pos_end = inner_block->pos_end;

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


    Llvm_goto* jmp_to_after_chain = goto_new(after_chain, old_block->pos);
    vec_append(&a_main, &new_block->children, llvm_wrap_goto(jmp_to_after_chain));

    vec_rem_last(&env->ancesters);
    return new_block;
}

static Llvm_block* if_else_chain_to_branch(Env* env, Node_if_else_chain* if_else) {
    Llvm_block* new_block = llvm_block_new(if_else->pos);

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

static Llvm_reg load_if_else_chain(
    Env* env,
    Llvm_block* new_block,
    Node_if_else_chain* old_if_else
) {
    Llvm_block* new_if_else = if_else_chain_to_branch(env, old_if_else);
    vec_append(&a_main, &new_block->children, llvm_wrap_block(new_if_else));

    return (Llvm_reg) {0};
}

static Llvm_block* for_range_to_branch(Env* env, Node_for_range* old_for) {
    Str_view old_if_break = env->label_if_break;
    Str_view old_if_continue = env->label_if_continue;

    size_t init_count_ancesters = env->ancesters.info.count;

    Pos pos = old_for->pos;

    Llvm_block* new_branch_block = llvm_block_new(pos);
    new_branch_block->symbol_collection = old_for->body->symbol_collection;
    new_branch_block->pos_end = old_for->body->pos_end;
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
        symbol_lhs_assign = util_symbol_new(for_var_def->name, for_var_def->pos);
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
        node_wrap_symbol_untyped(util_symbol_new(symbol_lhs_assign->name, symbol_lhs_assign->pos)),
        rhs_actual,
        TOKEN_LESS_THAN
    );

    // initial assignment

    Str_view check_cond_label = util_literal_name_new_prefix("check_cond");
    Str_view assign_label = util_literal_name_new_prefix("assign_for");
    Llvm_goto* jmp_to_check_cond_label = goto_new(check_cond_label, old_for->pos);
    Llvm_goto* jmp_to_assign = goto_new(assign_label, old_for->pos);
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

    log(LOG_DEBUG, STR_VIEW_FMT"\n", str_view_print(check_cond_label));
    assert(!alloca_lookup(&dummy, env, check_cond_label));

    {
        vec_rem_last(&env->ancesters);
        add_label(env, new_branch_block, check_cond_label, pos, false);
        vec_append(&a_main, &env->ancesters, &new_branch_block->symbol_collection);
    }

    symbol_log(LOG_DEBUG, env);
    log(LOG_DEBUG, STR_VIEW_FMT"\n", str_view_print(check_cond_label));
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

    log(LOG_DEBUG, "DSFJKLKJDFS: "STR_VIEW_FMT"\n", str_view_print(after_check_label));
    log(LOG_DEBUG, "DSFJKLKJDFS: "STR_VIEW_FMT"\n", str_view_print(check_cond_label));
    log(LOG_DEBUG, "DSFJKLKJDFS: "STR_VIEW_FMT"\n", str_view_print(after_for_loop_label));

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
    vec_append(&a_main, &new_branch_block->children, llvm_wrap_goto(goto_new(check_cond_label, pos)));
    add_label(env, new_branch_block, after_for_loop_label, pos, true);
    assert(alloca_lookup(&dummy, env, check_cond_label));

    try(alloca_do_add_defered(&dummy, env));

    assert(alloca_lookup(&dummy, env, check_cond_label));
    env->label_if_break = old_if_break;
    env->label_if_continue = old_if_continue;
    assert(alloca_lookup(&dummy, env, check_cond_label));
    //log(LOG_DEBUG, "DSFJKLKJDFS: "STR_VIEW_FMT"\n", str_view_print(check_cond_label));
    //symbol_log(LOG_DEBUG, env);
    vec_rem_last(&env->ancesters);

    assert(init_count_ancesters == env->ancesters.info.count);

    //log(LOG_DEBUG, "DSFJKLKJDFS: "STR_VIEW_FMT"\n", str_view_print(check_cond_label));
    //symbol_log(LOG_DEBUG, env);

    return new_branch_block;
}

static Llvm_reg load_for_range(
    Env* env,
    Llvm_block* new_block,
    Node_for_range* old_for
) {
    Llvm_block* new_for = for_range_to_branch(env, old_for);
    vec_append(&a_main, &new_block->children, llvm_wrap_block(new_for));

    return (Llvm_reg) {0};
}

static Llvm_block* for_with_cond_to_branch(Env* env, Node_for_with_cond* old_for) {
    Str_view old_if_break = env->label_if_break;
    Str_view old_if_continue = env->label_if_continue;

    Pos pos = old_for->pos;

    Llvm_block* new_branch_block = llvm_block_new(pos);
    new_branch_block->symbol_collection = old_for->body->symbol_collection;
    new_branch_block->pos_end = old_for->body->pos_end;

    vec_append(&a_main, &env->ancesters, &new_branch_block->symbol_collection);


    Node_operator* operator = old_for->condition->child;
    Str_view check_cond_label = util_literal_name_new_prefix("check_cond");
    Llvm_goto* jmp_to_check_cond_label = goto_new(check_cond_label, old_for->pos);
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
        goto_new(check_cond_label, old_for->pos)
    ));
    add_label(env, new_branch_block, after_for_loop_label, pos, true);

    Llvm* dummy = NULL;

    env->label_if_continue = old_if_continue;
    env->label_if_break = old_if_break;
    vec_rem_last(&env->ancesters);

    try(alloca_do_add_defered(&dummy, env));
    return new_branch_block;
}

static Llvm_reg load_for_with_cond(
    Env* env,
    Llvm_block* new_block,
    Node_for_with_cond* old_for
) {
    Llvm_block* new_for = for_with_cond_to_branch(env, old_for);
    vec_append(&a_main, &new_block->children, llvm_wrap_block(new_for));

    return (Llvm_reg) {0};
}

static Llvm_reg load_break(
    Env* env,
    Llvm_block* new_block,
    Node_break* old_break
) {
    // TODO: make expected failure test case for break in invalid location
    if (env->label_if_break.count < 1) {
        unreachable("cannot break here\n");
    }

    Llvm_goto* new_goto = llvm_goto_new(old_break->pos);
    new_goto->name = env->label_if_break;
    vec_append(&a_main, &new_block->children, llvm_wrap_goto(new_goto));

    return (Llvm_reg) {0};
}

static Llvm_reg load_continue(
    Env* env,
    Llvm_block* new_block,
    Node_continue* old_continue
) {
    // TODO: make expected failure test case for continue in invalid location
    if (env->label_if_continue.count < 1) {
        unreachable("cannot continue here\n");
    }

    Llvm_goto* new_goto = llvm_goto_new(old_continue->pos);
    new_goto->name = env->label_if_continue;
    vec_append(&a_main, &new_block->children, llvm_wrap_goto(new_goto));

    return (Llvm_reg) {0};
}

static Llvm_reg load_raw_union_def(
    Env* env,
    Llvm_block* new_block,
    Node_raw_union_def* old_def
) {
    (void) env;

    vec_append(&a_main, &new_block->children, llvm_wrap_def(
        llvm_wrap_raw_union_def(node_clone_raw_union_def(old_def))
    ));

    return (Llvm_reg) {0};
}

static Llvm_reg load_ptr_variable_def(
    Env* env,
    Llvm_block* new_block,
    Node_variable_def* old_variable_def
) {
    return load_variable_def(env, new_block, old_variable_def);
}

static Llvm_reg load_ptr_unary(
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
            
            Llvm_reg ptr = load_ptr_expr(env, new_block, old_unary->child);
    Llvm_load_another_llvm* new_load = llvm_load_another_llvm_new(pos);
            new_load->llvm_src = ptr;
            new_load->lang_type = old_unary->lang_type;
            new_load->lang_type.pointer_depth++;

            vec_append(&a_main, &new_block->children, llvm_wrap_load_another_llvm(new_load));
            return (Llvm_reg) {
                .lang_type = old_unary->lang_type,
                .llvm = llvm_wrap_load_another_llvm(new_load)
            };
        }
        default:
            todo();
    }
    todo();
}

static Llvm_reg load_ptr_operator(Env* env, Llvm_block* new_block, Node_operator* old_oper) {
    switch (old_oper->type) {
        case NODE_BINARY:
            todo();
        case NODE_UNARY:
            return load_ptr_unary(env, new_block, node_unwrap_unary(old_oper));
        default:
            unreachable("");
    }
}

static Llvm_reg load_ptr_expr(Env* env, Llvm_block* new_block, Node_expr* old_expr) {
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

static Llvm_reg load_ptr_def(Env* env, Llvm_block* new_block, Node_def* old_def) {
    switch (old_def->type) {
        case NODE_VARIABLE_DEF:
            return load_ptr_variable_def(env, new_block, node_unwrap_variable_def(old_def));
        default:
            unreachable("");
    }
}

static Llvm_reg load_ptr(Env* env, Llvm_block* new_block, Node* old_node) {
    switch (old_node->type) {
        case NODE_EXPR:
            return load_ptr_expr(env, new_block, node_unwrap_expr(old_node));
        case NODE_DEF:
            return load_ptr_def(env, new_block, node_unwrap_def(old_node));
        default:
            unreachable(NODE_FMT"\n", node_print(old_node));
    }
}

static Llvm_reg load_def(Env* env, Llvm_block* new_block, Node_def* old_def) {
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

static Llvm_reg load_node(Env* env, Llvm_block* new_block, Node* old_node) {
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
            return (Llvm_reg) {0};
        default:
            unreachable(NODE_FMT"\n", node_print(old_node));
    }
}

// TODO: rethink how to do block, because this is simply not working
static Llvm_block* load_block(Env* env, Node_block* old_block) {
    size_t init_count_ancesters = env->ancesters.info.count;

    Llvm_block* new_block = llvm_block_new(old_block->pos);
    new_block->is_variadic = old_block->is_variadic;
    new_block->symbol_collection = old_block->symbol_collection;
    new_block->pos_end = new_block->pos_end;

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

