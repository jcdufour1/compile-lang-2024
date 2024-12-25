#ifndef NODES_H
#define NODES_H

#include <stddef.h>
#include <string.h>
#include <strings.h>
#include "util.h"
#include <node.h>
#include <node_utils.h>
#include "assert.h"
#include "vector.h"

void node_print_assert_recursion_depth_zero(void);

#define log_tree(log_level, root) \
    do { \
        log_file_new(log_level, __FILE__, __LINE__, "tree:\n"NODE_FMT, node_print(root)); \
        node_print_assert_recursion_depth_zero(); \
    } while(0)

static inline Node* get_left_child_expr(Node_expr* expr) {
    (void) expr;
    unreachable("");
}

//static inline Node_expr* node_expr_new(Pos pos, NODE_EXPR_TYPE expr_type) {
//    Node_expr* expr = node_unwrap_expr(node_new(pos, NODE_EXPR));
//    expr->type = expr_type;
//    return expr;
//}

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
