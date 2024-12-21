#include <node.h>
#include <nodes.h>
#include <symbol_table.h>
#include <parser_utils.h>
#include <nodes.h>
#include "passes.h"

static Node_expr* id_expr(Node_expr* expr);
static Node_block* id_block(Node_block* block);

static Llvm_id llvm_id_new(void) {
    static Llvm_id llvm_id_for_next_var = 1;
    Llvm_id id_to_rtn = llvm_id_for_next_var;
    llvm_id_for_next_var += 2;
    return id_to_rtn;
}

static Node_unary* id_unary(Node_unary* unary) {
    unary->child = id_expr(unary->child);
    unary->llvm_id = llvm_id_new();
    return unary;
}
   

static Node_binary* id_binary(Node_binary* binary) {
    binary->lhs = id_expr(binary->lhs);
    binary->rhs = id_expr(binary->rhs);
    binary->llvm_id = llvm_id_new();
    return binary;
}

static Node_operator* id_operator(Node_operator* operator) {
    switch (operator->type) {
        case NODE_UNARY:
            return node_wrap_unary(id_unary(node_unwrap_unary(operator)));
        case NODE_BINARY:
            return node_wrap_binary(id_binary(node_unwrap_binary(operator)));
    }
    unreachable("");
}

static Node_function_call* id_function_call(Node_function_call* fun_call) {
    for (size_t idx = 0; idx < fun_call->args.info.count; idx++) {
        Node_expr** expr = vec_at_ref(&fun_call->args, idx);
        *expr = id_expr(*expr);
    }

    fun_call->llvm_id = llvm_id_new();

    return fun_call;
}

static Node_expr* id_expr(Node_expr* expr) {
    switch (expr->type) {
        case NODE_SYMBOL_UNTYPED:
            return expr;
        case NODE_MEMBER_ACCESS_UNTYPED:
            unreachable("");
        case NODE_MEMBER_ACCESS_TYPED:
            node_unwrap_member_access_typed(expr)->llvm_id = llvm_id_new();
            return expr;
        case NODE_INDEX_UNTYPED:
            unreachable("");
        case NODE_INDEX_TYPED:
            node_unwrap_index_typed(expr)->llvm_id = llvm_id_new();
            return expr;
        case NODE_STRUCT_LITERAL:
            return expr;
        case NODE_SYMBOL_TYPED:
            return expr;
        case NODE_LITERAL:
            return expr;
        case NODE_FUNCTION_CALL:
            return node_wrap_function_call(id_function_call(node_unwrap_function_call(expr)));
        case NODE_OPERATOR:
            return node_wrap_operator(id_operator(node_unwrap_operator(expr)));
        case NODE_LLVM_PLACEHOLDER:
            return expr;
    }
    unreachable("");
}

static Node_variable_def* id_variable_def(Node_variable_def* def) {
    def->llvm_id = llvm_id_new();
    return def;
}

static Node_function_params* id_function_params(Node_function_params* params) {
    for (size_t idx = 0; idx < params->params.info.count; idx++) {
        Node_variable_def** curr = vec_at_ref(&params->params, idx);
        *curr = id_variable_def(*curr);
    }

    return params;
}

static Node_function_decl* id_function_decl(Node_function_decl* fun_decl) {
    fun_decl->parameters = id_function_params(fun_decl->parameters);
    return fun_decl;
}

static Node_function_def* id_function_def(Node_function_def* fun_def) {
    fun_def->llvm_id = llvm_id_new();
    fun_def->declaration = id_function_decl(fun_def->declaration);
    fun_def->body = id_block(fun_def->body);
    return fun_def;
}

static Node_def* id_def(Node_def* def) {
    switch (def->type) {
        case NODE_VARIABLE_DEF:
            return node_wrap_variable_def(id_variable_def(node_unwrap_variable_def(def)));
        case NODE_LABEL:
            node_unwrap_label(def)->llvm_id = llvm_id_new();
            return def;
        case NODE_STRUCT_DEF:
            return def;
        case NODE_RAW_UNION_DEF:
            return def;
        case NODE_ENUM_DEF:
            return def;
        case NODE_FUNCTION_DECL:
            return def;
        case NODE_FUNCTION_DEF:
            return node_wrap_function_def(id_function_def(node_unwrap_function_def(def)));
        case NODE_PRIMITIVE_DEF:
            return def;
        case NODE_LITERAL_DEF:
            return def;
    }
    unreachable("");
}

static Node* id_node(Node* node) {
    switch (node->type) {
        case NODE_EXPR:
            return node_wrap_expr(id_expr(node_unwrap_expr(node)));
        case NODE_DEF:
            return node_wrap_def(id_def(node_unwrap_def(node)));
        case NODE_FUNCTION_PARAMS:
            return node;
        case NODE_BLOCK:
            return node_wrap_block(id_block(node_unwrap_block(node)));
        case NODE_RETURN:
            return node;
        case NODE_LANG_TYPE:
            return node;
        case NODE_LOAD_ELEMENT_PTR:
            node_unwrap_load_element_ptr(node)->llvm_id = llvm_id_new();
            return node;
        case NODE_LOAD_ANOTHER_NODE:
            node_unwrap_load_another_node(node)->llvm_id = llvm_id_new();
            return node;
        case NODE_STORE_ANOTHER_NODE:
            node_unwrap_store_another_node(node)->llvm_id = llvm_id_new();
            return node;
        case NODE_ASSIGNMENT:
            log_tree(LOG_ERROR, node);
            unreachable("");
        case NODE_ALLOCA:
            node_unwrap_alloca(node)->llvm_id = llvm_id_new();
            return node;
        case NODE_GOTO:
            node_unwrap_goto(node)->llvm_id = llvm_id_new();
            return node;
        case NODE_COND_GOTO:
            node_unwrap_cond_goto(node)->llvm_id = llvm_id_new();
            return node;
        case NODE_FOR_LOWER_BOUND:
            unreachable("");
        case NODE_FOR_UPPER_BOUND:
            unreachable("");
        case NODE_FOR_RANGE:
            unreachable("");
        case NODE_FOR_WITH_COND:
            unreachable("");
        case NODE_BREAK:
            unreachable("");
        case NODE_CONTINUE:
            unreachable("");
        case NODE_IF:
            unreachable("");
        case NODE_IF_ELSE_CHAIN:
            unreachable("");
        case NODE_CONDITION:
            unreachable("");
    }
    unreachable("");
}

static Node_block* id_block(Node_block* block) {
    for (size_t idx = 0; idx < block->children.info.count; idx++) {
        Node** curr = vec_at_ref(&block->children, idx);
        *curr = id_node(*curr);
    }

    return block;
}

Node_block* assign_llvm_ids(Env* env, Node_block* root) {
    (void) env;
    return id_block(root);
}
