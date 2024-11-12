#ifndef NODES_H
#define NODES_H

#include <stddef.h>
#include <string.h>
#include <strings.h>
#include "util.h"
#include "node.h"
#include "assert.h"
#include "vector.h"

void nodes_log_tree_internal(LOG_LEVEL log_level, const Node* root, const char* file, int line);

#define log_tree(log_level, root) \
    do { \
        log_file_new(log_level, __FILE__, __LINE__, "tree:\n"); \
        nodes_log_tree_internal(log_level, root, __FILE__, __LINE__); \
        log_file_new(log_level, __FILE__, __LINE__, "\n"); \
    } while(0)

static inline Node* get_left_child_expr(Node_expr* expr) {
    switch (expr->type) {
        case NODE_E_SYMBOL_UNTYPED:
            unreachable("");
        case NODE_E_SYMBOL_TYPED:
            unreachable("");
        case NODE_E_LITERAL:
            unreachable("");
        case NODE_E_MEMBER_SYM_UNTYPED:
            unreachable("");
        case NODE_E_MEMBER_SYM_TYPED:
            unreachable("");
        case NODE_E_FUNCTION_CALL:
            unreachable("");
        case NODE_E_STRUCT_LITERAL:
            unreachable("");
        case NODE_E_OPERATOR:
            unreachable("");
        case NODE_E_LLVM_PLACEHOLDER:
            unreachable("");
    }
    unreachable("");
}

static inline Node* get_left_child(Node* node) {
    switch (node->type) {
        case NODE_EXPR:
            return get_left_child_expr(node_unwrap_expr(node));
        case NODE_LABEL:
            unreachable("");
        case NODE_LANG_TYPE:
            unreachable("");
        case NODE_LOAD_ELEMENT_PTR:
            unreachable("");
        case NODE_VARIABLE_DEF:
            unreachable("");
        case NODE_MEMBER_SYM_PIECE_UNTYPED:
            unreachable("");
        case NODE_MEMBER_SYM_PIECE_TYPED:
            unreachable("");
        case NODE_STRUCT_DEF:
            unreachable("");
        case NODE_FUNCTION_PARAMS:
            unreachable("");
        case NODE_FUNCTION_DECL:
            unreachable("");
        case NODE_FUNCTION_DEF:
            unreachable("");
        case NODE_FOR_RANGE:
            unreachable("");
        case NODE_FOR_WITH_COND:
            unreachable("");
        case NODE_FOR_LOWER_BOUND:
            return node_wrap_expr(node_unwrap_for_lower_bound(node)->child);
        case NODE_FOR_UPPER_BOUND:
            return node_wrap_expr(node_unwrap_for_upper_bound(node)->child);
        case NODE_BREAK:
            return node_unwrap_break(node)->child;
        case NODE_RETURN:
            return node_wrap_expr(node_unwrap_return(node)->child);
        case NODE_ASSIGNMENT:
            unreachable("");
        case NODE_IF:
            unreachable("");
        case NODE_CONDITION:
            unreachable("");
        case NODE_BLOCK:
            unreachable("");
        case NODE_COND_GOTO:
            unreachable("");
        case NODE_GOTO:
            unreachable("");
        case NODE_ALLOCA:
            unreachable("");
        case NODE_LLVM_STORE_LITERAL:
            todo();
        case NODE_LOAD_ANOTHER_NODE:
            unreachable("");
        case NODE_STORE_ANOTHER_NODE:
            unreachable("");
        case NODE_LLVM_STORE_STRUCT_LITERAL:
            unreachable("");
        case NODE_PTR_BYVAL_SYM:
            return NULL;
        default:
            unreachable(NODE_FMT, node_print(node));
    }
}

static inline const Node* get_left_child_const(const Node* node) {
    return get_left_child((Node*)node);
}

static inline Node* node_new(Pos pos, NODE_TYPE node_type) {
    Node* new_node = arena_alloc(&a_main, sizeof(*new_node));
    if (!root_of_tree) {
        // it is assumed that the first node created is the root
        root_of_tree = new_node;
    }
    new_node->pos = pos;
    new_node->type = node_type;
    return new_node;
}

static inline Node_expr* node_expr_new(Pos pos, NODE_EXPR_TYPE expr_type) {
    Node_expr* expr = node_unwrap_expr(node_new(pos, NODE_EXPR));
    expr->type = expr_type;
    return expr;
}

static inline Node* node_clone(const Node* node_to_clone) {
    (void) node_to_clone;
    todo();
#if 0
    Node* new_node = node_new(node_to_clone->pos, node_to_clone->type);
    *new_node = *node_to_clone;
    nodes_reset_links_of_self_only(new_node, false);
    return new_node;
#endif // 0
}

#endif // NODES_H
