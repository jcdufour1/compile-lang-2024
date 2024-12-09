#include <node.h>
#include <nodes.h>
#include <symbol_table.h>
#include <parser_utils.h>

#include "passes.h"

static Node_block* load_block(Env* env, const Node_block* old_block);

static Llvm_register_sym load_expr(Env* env, Node_block* new_block, const Node_expr* old_expr);

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

    if (old_lit->type == NODE_STRING) {
        try(sym_tbl_add(&env->global_literals, node_wrap_expr(node_wrap_literal(new_lit))));
    }

    return (Llvm_register_sym) {
        .lang_type = old_lit->lang_type,
        .node = node_wrap_expr(node_wrap_literal(new_lit))
    };
}

static Llvm_register_sym load_expr(Env* env, Node_block* new_block, const Node_expr* old_expr) {
    switch (old_expr->type) {
        case NODE_FUNCTION_CALL:
            return load_function_call(env, new_block, node_unwrap_function_call_const(old_expr));
        case NODE_LITERAL:
            return load_literal(env, new_block, node_unwrap_literal_const(old_expr));
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


static Node_function_decl* node_clone_function_decl(const Node_function_decl* old_decl) {
    Node_function_decl* new_decl = node_function_decl_new(node_wrap_function_decl_const(old_decl)->pos);
    new_decl->parameters = node_clone_function_params(old_decl->parameters);
    new_decl->return_type = node_clone_lang_type(old_decl->return_type);
    new_decl->name = old_decl->name;
    return new_decl;
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
    load_function_parameters(env, new_fun_def->body, old_fun_def->declaration->parameters->params);
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
        default:
            unreachable(NODE_FMT"\n", node_print(old_node));
    }
}

static Node_block* load_block(Env* env, const Node_block* old_block) {
    if (old_block->children.info.count < 1) {
        // TODO: clone node instead of casting it
        return (Node_block*)old_block;
    }

    Node_block* new_block = node_block_new(node_wrap_block_const(old_block)->pos);
    *new_block = *old_block;
    memset(&new_block->children, 0, sizeof(new_block->children));

    for (size_t idx = 0; idx < old_block->children.info.count; idx++) {
        load_node(env, new_block, vec_at(&old_block->children, idx));
    }

    return new_block;
}

Node_block* add_load_and_store(Env* env, const Node_block* old_root) {
    return load_block(env, old_root);
}

