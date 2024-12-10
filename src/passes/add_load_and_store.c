#include <node.h>
#include <nodes.h>
#include <symbol_table.h>
#include <parser_utils.h>

#include "passes.h"

static Node_block* load_block(Env* env, const Node_block* old_block);

static Llvm_register_sym load_expr(Env* env, Node_block* new_block, const Node_expr* old_expr);

static Llvm_register_sym load_ptr(Env* env, Node_block* new_block, const Node* old_node);

static Llvm_register_sym load_ptr_expr(Env* env, Node_block* new_block, const Node_expr* old_expr);

static Llvm_register_sym load_operator(
    Env* env,
    Node_block* new_block,
    const Node_operator* old_oper
);

static Node_goto* goto_new(Str_view name_label_to_jmp_to, Pos pos) {
    Node_goto* lang_goto = node_goto_new(pos);
    lang_goto->name = name_label_to_jmp_to;
    return lang_goto;
}

static void add_label(Env* env, Node_block* block, Str_view label_name, Pos pos) {
    Node_label* label = node_label_new(pos);
    label->name = label_name;
    try(symbol_add(env, node_wrap_label(label)));

    vec_append(&a_main, &block->children, node_wrap_label(label));
}

static void if_for_add_cond_goto(
    Env* env,
    const Node_operator* old_oper,
    Node_block* block,
    Str_view label_name_if_true,
    Str_view label_name_if_false
) {
    Pos pos = node_wrap_expr_const(node_wrap_operator_const(old_oper))->pos;

    Node_cond_goto* cond_goto = node_cond_goto_new(pos);
    cond_goto->node_src = load_operator(env, block, old_oper);
    cond_goto->if_true = symbol_new(label_name_if_true, pos);
    cond_goto->if_false = symbol_new(label_name_if_false, pos);

    vec_append(&a_main, &block->children, node_wrap_cond_goto(cond_goto));
}

static Node_assignment* for_loop_cond_var_assign_new(Env* env, Str_view sym_name, Pos pos) {
    Node_literal* literal = literal_new(str_view_from_cstr("1"), TOKEN_INT_LITERAL, pos);
    Node_operator* operator = util_binary_typed_new(env, node_wrap_symbol_untyped(symbol_new(sym_name, pos)), node_wrap_literal(literal), TOKEN_SINGLE_PLUS);
    return assignment_new(env, node_wrap_expr(node_wrap_symbol_untyped(symbol_new(sym_name, pos))), node_wrap_operator(operator));
}

static Llvm_register_sym load_function_call(
    Env* env,
    Node_block* new_block,
    const Node_function_call* old_fun_call
) {

    Node_function_call* new_fun_call = node_function_call_new(
        node_wrap_expr(node_wrap_function_call_const(old_fun_call))->pos
    );

    for (size_t idx = 0; idx < old_fun_call->args.info.count; idx++) {
        const Node_expr* old_arg = vec_at(&old_fun_call->args, idx);
        Pos arg_pos = node_wrap_expr_const(old_arg)->pos;

        Node_llvm_placeholder* new_arg = node_llvm_placeholder_new(arg_pos);
        new_arg->lang_type = get_lang_type_expr(old_arg);
        new_arg->llvm_reg = load_expr(env, new_block, old_arg);
        vec_append(&a_main, &new_fun_call->args, node_wrap_llvm_placeholder(new_arg));
    }

    new_fun_call->name = old_fun_call->name;
    new_fun_call->lang_type = old_fun_call->lang_type;
    
    vec_append(&a_main, &new_block->children, node_wrap_expr(node_wrap_function_call(new_fun_call)));
    return (Llvm_register_sym) {
        .lang_type = old_fun_call->lang_type,
        .node = node_wrap_expr(node_wrap_function_call(new_fun_call))
    };
}

static Node_literal* node_literal_clone(const Node_literal* old_lit) {
    Node_literal* new_lit = node_literal_new(node_wrap_expr_const(node_wrap_literal_const(old_lit))->pos);
    *new_lit = *old_lit;
    return new_lit;
}

static Llvm_register_sym load_literal(
    Env* env,
    Node_block* new_block,
    const Node_literal* old_lit
) {
    (void) env;
    (void) new_block;

    Node_literal* new_lit = node_literal_clone(old_lit);

    if (new_lit->type == NODE_STRING) {
        try(sym_tbl_add(&env->global_literals, node_wrap_expr(node_wrap_literal(new_lit))));
    }

    return (Llvm_register_sym) {
        .lang_type = old_lit->lang_type,
        .node = node_wrap_expr(node_wrap_literal(new_lit))
    };
}
static Llvm_register_sym load_ptr_symbol_typed(
    Env* env,
    Node_block* new_block,
    const Node_symbol_typed* old_sym
) {
    (void) new_block;

    Node* var_def_ = NULL;
    try(symbol_lookup(&var_def_, env, old_sym->name));
    Node_variable_def* var_def = node_unwrap_variable_def(var_def_);
    assert(var_def);
    assert(lang_type_is_equal(var_def->lang_type, old_sym->lang_type));

    assert(var_def->storage_location.node);
    return var_def->storage_location;
}

static Llvm_register_sym load_symbol_typed(
    Env* env,
    Node_block* new_block,
    const Node_symbol_typed* old_sym
) {
    Pos pos = node_wrap_expr(node_wrap_symbol_typed_const(old_sym))->pos;

    Node_load_another_node* new_load = node_load_another_node_new(pos);
    new_load->node_src = load_ptr_symbol_typed(env, new_block, old_sym);
    new_load->lang_type = old_sym->lang_type;

    vec_append(&a_main, &new_block->children, node_wrap_load_another_node(new_load));
    return (Llvm_register_sym) {
        .lang_type = old_sym->lang_type,
        .node = node_wrap_load_another_node(new_load)
    };
}

static Llvm_register_sym load_binary(
    Env* env,
    Node_block* new_block,
    const Node_binary* old_bin
) {
    Pos pos = node_wrap_expr(node_wrap_operator(node_wrap_binary_const(old_bin)))->pos;

    Node_binary* new_bin = node_binary_new(pos);

    Node_llvm_placeholder* lhs = node_llvm_placeholder_new(pos);
    lhs->lang_type = get_lang_type_expr(old_bin->lhs);
    lhs->llvm_reg = load_expr(env, new_block, old_bin->lhs);
    new_bin->lhs = node_wrap_llvm_placeholder(lhs);

    Node_llvm_placeholder* rhs = node_llvm_placeholder_new(pos);
    rhs->lang_type = get_lang_type_expr(old_bin->rhs);
    rhs->llvm_reg = load_expr(env, new_block, old_bin->rhs);
    new_bin->rhs = node_wrap_llvm_placeholder(rhs);

    new_bin->token_type = old_bin->token_type;
    new_bin->lang_type = old_bin->lang_type;

    vec_append(&a_main, &new_block->children, node_wrap_expr(node_wrap_operator(node_wrap_binary(new_bin))));
    return (Llvm_register_sym) {
        .lang_type = new_bin->lang_type,
        .node = node_wrap_expr(node_wrap_operator(node_wrap_binary(new_bin)))
    };

}

static Llvm_register_sym load_unary(
    Env* env,
    Node_block* new_block,
    const Node_unary* old_unary
) {
    (void) env;
    (void) new_block;
    (void) old_unary;
    todo();
}

static Llvm_register_sym load_operator(
    Env* env,
    Node_block* new_block,
    const Node_operator* old_oper
) {
    switch (old_oper->type) {
        case NODE_BINARY:
            return load_binary(env, new_block, node_unwrap_binary_const(old_oper));
        case NODE_UNARY:
            return load_unary(env, new_block, node_unwrap_unary_const(old_oper));
        default:
            unreachable("");
    }
}

// TODO: clone in this function
static Llvm_register_sym do_load_struct_element_ptr(
    Env* env,
    Node_block* new_block,
    const Node_member_sym_typed* symbol_call
) {
    Llvm_register_sym result;

    Node* prev_struct_sym = node_wrap_expr(node_wrap_member_sym_typed_const(symbol_call));
    Llvm_register_sym node_element_ptr_to_load = get_storage_location(env, symbol_call->name);

    Node* struct_var_def = NULL;
    if (!symbol_lookup(&struct_var_def, env, symbol_call->name)) {
        todo();
    }
    Lang_type type_load_from = get_lang_type(struct_var_def);

    Node_load_element_ptr* load_element_ptr = NULL;
    for (size_t idx = 0; idx < symbol_call->children.info.count; idx++) {
        Node* element_sym_ = vec_at(&symbol_call->children, idx);
        Node_member_sym_piece_typed* element_sym = node_unwrap_member_sym_piece_typed(element_sym_);

        //log(LOG_DEBUG, LANG_TYPE_FMT"\n", lang_type_print(type_load_from));
        log(LOG_DEBUG, LANG_TYPE_FMT"\n", lang_type_print(type_load_from));
        log(LOG_DEBUG, LANG_TYPE_FMT"\n", lang_type_print(element_sym->lang_type));
        log_tree(LOG_DEBUG, node_wrap_expr(node_wrap_member_sym_typed_const(symbol_call)));
        log_tree(LOG_DEBUG, element_sym_);

        if (lang_type_is_struct(env, type_load_from)) {
            load_element_ptr = node_load_element_ptr_new(node_wrap_member_sym_piece_typed(element_sym)->pos);
            load_element_ptr->name = get_node_name(prev_struct_sym);
            load_element_ptr->lang_type = element_sym->lang_type;
            load_element_ptr->struct_index = element_sym->struct_index;
            load_element_ptr->node_src = node_element_ptr_to_load;
            vec_append(&a_main, &new_block->children, node_wrap_load_element_ptr(load_element_ptr));
        }

        type_load_from = element_sym->lang_type;
        prev_struct_sym = node_wrap_member_sym_piece_typed(element_sym);
        if (load_element_ptr) {
            node_element_ptr_to_load = llvm_register_sym_new(node_wrap_load_element_ptr(load_element_ptr));
        } else {
            memset(&node_element_ptr_to_load, 0, sizeof(node_element_ptr_to_load));
        }
    }

    if (load_element_ptr) {
        result = llvm_register_sym_new(node_wrap_load_element_ptr(load_element_ptr));
        result.lang_type = load_element_ptr->lang_type;
        result.node = node_wrap_load_element_ptr(load_element_ptr);
        return result;
    } else {
        result = get_storage_location(env, get_expr_name(node_wrap_member_sym_typed_const(symbol_call)));
        Node* struct_var_def = NULL;
        if (!symbol_lookup(&struct_var_def, env, get_expr_name(node_wrap_member_sym_typed_const(symbol_call)))) {
            todo();
        }
        result.lang_type = get_member_sym_piece_final_lang_type(symbol_call);
        return result;
    }
    unreachable("");
}

static Llvm_register_sym load_ptr_member_sym_typed(
    Env* env,
    Node_block* new_block,
    const Node_member_sym_typed* old_memb_sym
) {
    return do_load_struct_element_ptr(env, new_block, old_memb_sym);
}

static Llvm_register_sym load_member_sym_typed(
    Env* env,
    Node_block* new_block,
    const Node_member_sym_typed* old_memb_sym
) {
    Pos pos = node_wrap_expr(node_wrap_member_sym_typed_const(old_memb_sym))->pos;

    Llvm_register_sym ptr = load_ptr_member_sym_typed(env, new_block, old_memb_sym);

    Node_load_another_node* new_load = node_load_another_node_new(pos);
    new_load->node_src = ptr;
    new_load->lang_type = ptr.lang_type;

    vec_append(&a_main, &new_block->children, node_wrap_load_another_node(new_load));
    return (Llvm_register_sym) {
        .lang_type = new_load->lang_type,
        .node = node_wrap_load_another_node(new_load)
    };
}

static Llvm_register_sym load_expr(Env* env, Node_block* new_block, const Node_expr* old_expr) {
    switch (old_expr->type) {
        case NODE_FUNCTION_CALL:
            return load_function_call(env, new_block, node_unwrap_function_call_const(old_expr));
        case NODE_LITERAL:
            return load_literal(env, new_block, node_unwrap_literal_const(old_expr));
        case NODE_SYMBOL_TYPED:
            return load_symbol_typed(env, new_block, node_unwrap_symbol_typed_const(old_expr));
        case NODE_SYMBOL_UNTYPED:
            unreachable("NODE_SYMBOL_UNTYPED should not still be present at this point");
        case NODE_OPERATOR:
            return load_operator(env, new_block, node_unwrap_operator_const(old_expr));
        case NODE_MEMBER_SYM_TYPED:
            return load_member_sym_typed(env, new_block, node_unwrap_member_sym_typed_const(old_expr));
        default:
            unreachable(NODE_FMT"\n", node_print(node_wrap_expr_const(old_expr)));
    }
}

static Node_function_params* node_clone_function_params(const Node_function_params* old_params) {
    Node_function_params* new_params = node_function_params_new(node_wrap_function_params_const(old_params)->pos);
    *new_params = *old_params;
    return new_params;
}

static Node_lang_type* node_clone_lang_type(const Node_lang_type* old_lang_type) {
    Node_lang_type* new_lang_type = node_lang_type_new(node_wrap_lang_type_const(old_lang_type)->pos);
    *new_lang_type = *old_lang_type;
    return new_lang_type;
}

static Node_alloca* node_clone_alloca(const Node_alloca* old_alloca) {
    Node_alloca* new_alloca = node_alloca_new(node_wrap_alloca_const(old_alloca)->pos);
    *new_alloca = *old_alloca;
    return new_alloca;
}

static Node_function_decl* node_clone_function_decl(const Node_function_decl* old_decl) {
    Node_function_decl* new_decl = node_function_decl_new(node_wrap_function_decl_const(old_decl)->pos);
    new_decl->parameters = node_clone_function_params(old_decl->parameters);
    new_decl->return_type = node_clone_lang_type(old_decl->return_type);
    new_decl->name = old_decl->name;
    return new_decl;
}

static Node_variable_def* node_clone_variable_def(const Node_variable_def* old_var_def) {
    Node_variable_def* new_var_def = node_variable_def_new(node_wrap_variable_def_const(old_var_def)->pos);
    *new_var_def = *old_var_def;
    return new_var_def;
}

static size_t get_idx_node_after_last_alloca(const Node_block* block) {
    for (size_t idx = 0; idx < block->children.info.count; idx++) {
        Node* node = vec_at(&block->children, idx);
        if (node->type != NODE_ALLOCA) {
            return idx;
        }
    }
    unreachable("");
}

static void load_function_parameters(
    Env* env,
    Node_block* new_fun_body,
    Node_ptr_vec fun_params
) {
    for (size_t idx = 0; idx < fun_params.info.count; idx++) {
        assert(env->ancesters.info.count > 0);
        Node* param = vec_at(&fun_params, idx);
        if (is_corresponding_to_a_struct(env, param)) {
            continue;
        }
        Llvm_register_sym fun_param_call = llvm_register_sym_new(param);
        (void) fun_param_call;
        size_t idx_insert_before = get_idx_node_after_last_alloca(new_fun_body);
        (void) idx_insert_before;
        //insert_store(env, &fun_def->body->children, &idx_insert_before, fun_param_call.node);
        todo();
    }
}

static Llvm_register_sym load_function_def(
    Env* env,
    Node_block* new_block,
    const Node_function_def* old_fun_def
) {
    Node_function_def* new_fun_def = node_function_def_new(node_wrap_function_def_const(old_fun_def)->pos);
    new_fun_def->declaration = node_clone_function_decl(old_fun_def->declaration);
    new_fun_def->body = node_block_new(node_wrap_block(old_fun_def->body)->pos);

    {
        vec_append(&a_main, &env->ancesters, node_wrap_block(old_fun_def->body));
        load_function_parameters(env, new_fun_def->body, old_fun_def->declaration->parameters->params);
        vec_rem_last(&env->ancesters);
    }

    vec_extend(&a_main, &new_fun_def->body->children, &old_fun_def->body->children);
    new_fun_def->body->symbol_table = old_fun_def->body->symbol_table;
    new_fun_def->body = load_block(env, new_fun_def->body);

    vec_append(&a_main, &new_block->children, node_wrap_function_def(new_fun_def));
    return (Llvm_register_sym) {
        .lang_type = old_fun_def->declaration->return_type->lang_type,
        .node = node_wrap_function_def(new_fun_def)
    };
}

static Llvm_register_sym load_function_decl(
    Env* env,
    Node_block* new_block,
    const Node_function_decl* old_fun_decl
) {
    (void) env;

    Node_function_decl* new_fun_decl = node_clone_function_decl(old_fun_decl);

    vec_append(&a_main, &new_block->children, node_wrap_function_decl(new_fun_decl));
    return (Llvm_register_sym) {
        .lang_type = old_fun_decl->return_type->lang_type,
        .node = node_wrap_function_decl(new_fun_decl)
    };
}

static Llvm_register_sym load_return(
    Env* env,
    Node_block* new_block,
    const Node_return* old_return
) {
    Pos pos = node_wrap_return_const(old_return)->pos;

    Node_return* new_return = node_return_new(pos);
    Llvm_register_sym result = load_expr(env, new_block, old_return->child);
    Node_llvm_placeholder* new_child = node_llvm_placeholder_new(pos);
    new_child->llvm_reg = result;
    new_child->lang_type = result.lang_type;
    new_return->child = node_wrap_llvm_placeholder(new_child);

    vec_append(&a_main, &new_block->children, node_wrap_return(new_return));


    return (Llvm_register_sym) {0};
}

static Llvm_register_sym load_alloca(
    Env* env,
    Node_block* new_block,
    const Node_alloca* old_alloca
) {
    (void) env;

    // TODO: clone alloca
    // make id system to make this actually possible
    //Node_alloca* new_alloca = node_clone_alloca(old_alloca);

    vec_append(&a_main, &new_block->children, node_wrap_alloca((Node_alloca*)old_alloca));

    return (Llvm_register_sym) {
        .lang_type = old_alloca->lang_type,
        .node = node_wrap_alloca((Node_alloca*)old_alloca)
    };
}

static Llvm_register_sym load_assignment(
    Env* env,
    Node_block* new_block,
    const Node_assignment* old_assignment
) {
    assert(old_assignment->lhs);
    assert(old_assignment->rhs);

    (void) env;
    Pos pos = node_wrap_assignment_const(old_assignment)->pos;

    Node_store_another_node* new_store = node_store_another_node_new(pos);
    new_store->lang_type = get_lang_type(old_assignment->lhs);
    new_store->node_src = load_expr(env, new_block, old_assignment->rhs);
    new_store->node_dest = load_ptr(env, new_block, old_assignment->lhs);

    assert(new_store->node_src.node);
    assert(new_store->node_dest.node);
    assert(old_assignment->rhs);

    vec_append(&a_main, &new_block->children, node_wrap_store_another_node(new_store));

    return (Llvm_register_sym) {
        .lang_type = new_store->lang_type,
        .node = node_wrap_store_another_node(new_store)
    };
}

static Llvm_register_sym load_variable_def(
    Env* env,
    Node_block* new_block,
    const Node_variable_def* old_var_def
) {
    (void) env;

    Node_variable_def* new_var_def = node_clone_variable_def(old_var_def);
    vec_append(&a_main, &new_block->children, node_wrap_variable_def(new_var_def));

    assert(new_var_def->storage_location.node);
    return new_var_def->storage_location;
}

static Llvm_register_sym load_goto(
    Env* env,
    Node_block* new_block,
    const Node_goto* old_goto
) {
    unreachable("");
    (void) env;

    // TODO: clone
    vec_append(&a_main, &new_block->children, (Node*)old_goto);

    return (Llvm_register_sym) {0};
}

static Llvm_register_sym load_cond_goto(
    Env* env,
    Node_block* new_block,
    const Node_cond_goto* old_cond_goto
) {
    unreachable("");
    (void) env;

    // TODO: clone
    vec_append(&a_main, &new_block->children, (Node*)old_cond_goto);

    return (Llvm_register_sym) {0};
}

static Llvm_register_sym load_label(
    Env* env,
    Node_block* new_block,
    const Node_label* old_label
) {
    unreachable("");
    (void) env;

    // TODO: clone
    vec_append(&a_main, &new_block->children, (Node*)old_label);

    return (Llvm_register_sym) {0};
}

static Llvm_register_sym load_struct_def(
    Env* env,
    Node_block* new_block,
    const Node_struct_def* old_struct_def
) {
    (void) env;

    // TODO: clone
    vec_append(&a_main, &new_block->children, (Node*)old_struct_def);

    return (Llvm_register_sym) {0};
}

static Llvm_register_sym load_enum_def(
    Env* env,
    Node_block* new_block,
    const Node_enum_def* old_enum_def
) {
    (void) env;

    // TODO: clone
    vec_append(&a_main, &new_block->children, (Node*)old_enum_def);

    return (Llvm_register_sym) {0};
}

static Node_block* if_statement_to_branch(Env* env, Node_if* if_statement, Str_view next_if, Str_view after_chain) {
    Node_condition* if_cond = if_statement->condition;
    Node_block* old_block = if_statement->body;

    Node_block* new_block = node_block_new(node_wrap_block(old_block)->pos);

    Node_operator* old_oper = if_cond->child;

    Str_view if_body = literal_name_new();

    if_for_add_cond_goto(env, old_oper, new_block, if_body, next_if);

    add_label(env, new_block, if_body, node_wrap_block(old_block)->pos);

    {
        Node_block* inner_block = load_block(env, old_block);
        new_block->symbol_table = inner_block->symbol_table;
        new_block->pos_end = inner_block->pos_end;
        vec_extend(&a_main, &new_block->children, &inner_block->children);
    }

    Node_goto* jmp_to_after_chain = goto_new(after_chain, node_wrap_block(old_block)->pos);
    vec_append(&a_main, &new_block->children, node_wrap_goto(jmp_to_after_chain));

    return new_block;
}

static Node_block* if_else_chain_to_branch(Env* env, const Node_if_else_chain* if_else) {
    Node_block* new_block = node_block_new(node_wrap_if_else_chain_const(if_else)->pos);

    Str_view if_after = literal_name_new();

    Str_view next_if = {0};
    for (size_t idx = 0; idx < if_else->nodes.info.count; idx++) {
        if (idx + 1 == if_else->nodes.info.count) {
            next_if = if_after;
        } else {
            next_if = literal_name_new();
        }

        Node_block* if_block = if_statement_to_branch(env, vec_at(&if_else->nodes, idx), next_if, if_after);
        vec_append(&a_main, &new_block->children, node_wrap_block(if_block));

        if (idx + 1 < if_else->nodes.info.count) {
            add_label(env, new_block, next_if, node_wrap_if(vec_at(&if_else->nodes, idx))->pos);
        }
    }

    add_label(env, new_block, if_after, node_wrap_if_else_chain_const(if_else)->pos);
    log_tree(LOG_DEBUG, node_wrap_block(new_block));

    return new_block;
}

static Llvm_register_sym load_if_else_chain(
    Env* env,
    Node_block* new_block,
    const Node_if_else_chain* old_if_else
) {
    Node_block* new_if_else = if_else_chain_to_branch(env, old_if_else);
    vec_append(&a_main, &new_block->children, node_wrap_block(new_if_else));

    return (Llvm_register_sym) {0};
}

static Node_block* for_range_to_branch(Env* env, const Node_for_range* for_loop) {

    Pos pos = node_wrap_for_range_const(for_loop)->pos;
    (void) pos;

    log_tree(LOG_TRACE, node_wrap_for_range_const(for_loop));

    Node_block* for_block = for_loop->body;
    //vec_append(&a_main, &env->ancesters, node_wrap_block(for_block));
    symbol_log(LOG_DEBUG, env);
    Node_expr* lhs_actual = for_loop->lower_bound->child;
    Node_expr* rhs_actual = for_loop->upper_bound->child;

    Node_block* new_branch_block = node_block_new(node_wrap_for_range_const(for_loop)->pos);

    Node_symbol_untyped* symbol_lhs_assign;
    Node_variable_def* for_var_def;
    {
        for_var_def = for_loop->var_def;
        symbol_lhs_assign = symbol_new(for_var_def->name, node_wrap_variable_def(for_var_def)->pos);
    }

    symbol_add(env, node_wrap_variable_def(for_var_def));
    Node* dummy = NULL;
    assert(symbol_lookup(&dummy, env, for_var_def->name));
    symbol_log(LOG_DEBUG, env);

    Node_assignment* assignment_to_inc_cond_var = for_loop_cond_var_assign_new(env, for_var_def->name, node_wrap_expr(lhs_actual)->pos);

    Node_operator* operator = util_binary_typed_new(
        env, node_wrap_symbol_untyped(symbol_new(symbol_lhs_assign->name, node_wrap_expr(node_wrap_symbol_untyped(symbol_lhs_assign))->pos)), rhs_actual, TOKEN_LESS_THAN
    );

    // initial assignment
    log_tree(LOG_DEBUG, (Node*)symbol_lhs_assign);
    log_tree(LOG_DEBUG, (Node*)lhs_actual);
    Node_assignment* new_var_assign = assignment_new(env, node_wrap_expr(node_wrap_symbol_untyped(symbol_lhs_assign)), lhs_actual);
    log_tree(LOG_DEBUG, (Node*)new_var_assign);

    Str_view check_cond_label = literal_name_new();
    Node_goto* jmp_to_check_cond_label = goto_new(check_cond_label, node_wrap_for_range_const(for_loop)->pos);
    Str_view after_check_label = literal_name_new();
    Str_view after_for_loop_label = literal_name_new();

    // TODO: implement this
    //change_break_to_goto(for_block, after_for_loop_label);

    vec_append(&a_main, &new_branch_block->children, node_wrap_alloca(
        add_alloca_alloca_new(for_var_def)
    ));
    vec_append(&a_main, &new_branch_block->children, node_wrap_variable_def(for_var_def));
    log(LOG_DEBUG, "%zu\n", env->ancesters.info.count);

    load_assignment(env, new_branch_block, new_var_assign);
    vec_append(&a_main, &new_branch_block->children, node_wrap_goto(jmp_to_check_cond_label));

    add_label(env, new_branch_block, check_cond_label, pos);
    load_operator(env, new_branch_block, operator);

    if_for_add_cond_goto(
        env,
        operator,
        new_branch_block,
        after_check_label,
        after_for_loop_label
    );

    add_label(env, new_branch_block, after_check_label, pos);

    {
        Node_block* inner_block = load_block(env, for_block);
        new_branch_block->symbol_table = inner_block->symbol_table;
        new_branch_block->pos_end = inner_block->pos_end;
        vec_extend(&a_main, &new_branch_block->children, &inner_block->children);
    }

    load_assignment(env, new_branch_block, assignment_to_inc_cond_var);
    vec_append(&a_main, &new_branch_block->children, node_wrap_goto(goto_new(check_cond_label, node_wrap_for_range_const(for_loop)->pos)));
    add_label(env, new_branch_block, after_for_loop_label, pos);

    //vec_rem_last(&env->ancesters);
    return new_branch_block;
}

static Llvm_register_sym load_for_range(
    Env* env,
    Node_block* new_block,
    const Node_for_range* old_for
) {
    Node_block* new_for = for_range_to_branch(env, old_for);
    vec_append(&a_main, &new_block->children, node_wrap_block(new_for));

    return (Llvm_register_sym) {0};
}

static Node_block* for_with_cond_to_branch(Env* env, const Node_for_with_cond* old_for) {
    Pos pos = node_wrap_for_with_cond_const(old_for)->pos;
    Node_block* new_branch_block = node_block_new(pos);

    Node_operator* operator = old_for->condition->child;
    Str_view check_cond_label = literal_name_new();
    Node_goto* jmp_to_check_cond_label = goto_new(check_cond_label, node_wrap_for_with_cond_const(old_for)->pos);
    Str_view after_check_label = literal_name_new();
    Str_view after_for_loop_label = literal_name_new();
    Llvm_register_sym oper_rtn_sym = llvm_register_sym_new_from_expr(node_wrap_operator(operator));
    oper_rtn_sym.node = node_wrap_expr(node_wrap_operator(operator));

    // TODO: implement this
    //change_break_to_goto(for_block, after_for_loop_label);

    vec_append(&a_main, &new_branch_block->children, node_wrap_goto(jmp_to_check_cond_label));
    add_label(env, new_branch_block, check_cond_label, pos);
    vec_append(&a_main, &new_branch_block->children, node_wrap_expr(node_wrap_operator(operator)));

    if_for_add_cond_goto(
        env,
        operator,
        new_branch_block,
        after_check_label,
        after_for_loop_label
    );

    add_label(env, new_branch_block, after_check_label, pos);

    {
        Node_block* inner_block = load_block(env, old_for->body);
        new_branch_block->symbol_table = inner_block->symbol_table;
        new_branch_block->pos_end = inner_block->pos_end;
        vec_extend(&a_main, &new_branch_block->children, &inner_block->children);
    }


    vec_append(&a_main, &new_branch_block->children, node_wrap_goto(
        goto_new(check_cond_label, node_wrap_for_with_cond_const(old_for)->pos)
    ));
    add_label(env, new_branch_block, after_for_loop_label, pos);

    new_branch_block->symbol_table = old_for->body->symbol_table;
    return new_branch_block;
}

static Llvm_register_sym load_for_with_cond(
    Env* env,
    Node_block* new_block,
    const Node_for_with_cond* old_for
) {
    Node_block* new_for = for_with_cond_to_branch(env, old_for);
    vec_append(&a_main, &new_block->children, node_wrap_block(new_for));

    return (Llvm_register_sym) {0};
}

static Llvm_register_sym load_ptr_variable_def(
    Env* env,
    Node_block* new_block,
    const Node_variable_def* old_variable_def
) {
    return load_variable_def(env, new_block, old_variable_def);
}

static Llvm_register_sym load_ptr_expr(Env* env, Node_block* new_block, const Node_expr* old_expr) {
    switch (old_expr->type) {
        case NODE_SYMBOL_TYPED:
            return load_ptr_symbol_typed(env, new_block, node_unwrap_symbol_typed_const(old_expr));
        case NODE_MEMBER_SYM_TYPED:
            return load_ptr_member_sym_typed(env, new_block, node_unwrap_member_sym_typed_const(old_expr));
        default:
            unreachable(NODE_FMT"\n", node_print(node_wrap_expr_const(old_expr)));
    }
}

static Llvm_register_sym load_ptr(Env* env, Node_block* new_block, const Node* old_node) {
    switch (old_node->type) {
        case NODE_EXPR:
            return load_ptr_expr(env, new_block, node_unwrap_expr_const(old_node));
        case NODE_VARIABLE_DEF:
            return load_ptr_variable_def(env, new_block, node_unwrap_variable_def_const(old_node));
        default:
            unreachable(NODE_FMT"\n", node_print(old_node));
    }
}

static Llvm_register_sym load_node(Env* env, Node_block* new_block, const Node* old_node) {
    switch (old_node->type) {
        case NODE_EXPR:
            return load_expr(env, new_block, node_unwrap_expr_const(old_node));
        case NODE_FUNCTION_DEF:
            return load_function_def(env, new_block, node_unwrap_function_def_const(old_node));
        case NODE_FUNCTION_DECL:
            return load_function_decl(env, new_block, node_unwrap_function_decl_const(old_node));
        case NODE_RETURN:
            return load_return(env, new_block, node_unwrap_return_const(old_node));
        case NODE_ALLOCA:
            return load_alloca(env, new_block, node_unwrap_alloca_const(old_node));
        case NODE_ASSIGNMENT:
            return load_assignment(env, new_block, node_unwrap_assignment_const(old_node));
        case NODE_VARIABLE_DEF:
            return load_variable_def(env, new_block, node_unwrap_variable_def_const(old_node));
        case NODE_GOTO:
            return load_goto(env, new_block, node_unwrap_goto_const(old_node));
        case NODE_COND_GOTO:
            return load_cond_goto(env, new_block, node_unwrap_cond_goto_const(old_node));
        case NODE_LABEL:
            return load_label(env, new_block, node_unwrap_label_const(old_node));
        case NODE_STRUCT_DEF:
            return load_struct_def(env, new_block, node_unwrap_struct_def_const(old_node));
        case NODE_IF_ELSE_CHAIN:
            return load_if_else_chain(env, new_block, node_unwrap_if_else_chain_const(old_node));
        case NODE_FOR_RANGE:
            return load_for_range(env, new_block, node_unwrap_for_range_const(old_node));
        case NODE_FOR_WITH_COND:
            return load_for_with_cond(env, new_block, node_unwrap_for_with_cond_const(old_node));
        case NODE_ENUM_DEF:
            return load_enum_def(env, new_block, node_unwrap_enum_def_const(old_node));
        case NODE_BREAK:
            return load_break(env, new_block, node_unwrap_enum_def_const(old_node));
        case NODE_BLOCK:
            vec_append(
                &a_main,
                &new_block->children,
                node_wrap_block(load_block(env, node_unwrap_block_const(old_node)))
            );
            return (Llvm_register_sym) {0};
        default:
            unreachable(NODE_FMT"\n", node_print(old_node));
    }
}

static Node_block* load_block(Env* env, const Node_block* old_block) {
    size_t init_count_ancesters = env->ancesters.info.count;

    Node_block* new_block = node_block_new(node_wrap_block_const(old_block)->pos);
    *new_block = *old_block;
    memset(&new_block->children, 0, sizeof(new_block->children));

    vec_append(&a_main, &env->ancesters, node_wrap_block(new_block));

    for (size_t idx = 0; idx < old_block->children.info.count; idx++) {
        load_node(env, new_block, vec_at(&old_block->children, idx));
    }

    vec_rem_last(&env->ancesters);

    try(init_count_ancesters == env->ancesters.info.count);
    return new_block;
}

Node_block* add_load_and_store(Env* env, const Node_block* old_root) {
    return load_block(env, old_root);
}

