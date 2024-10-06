#include "passes.h"
#include "../node.h"
#include "../nodes.h"
#include "../symbol_table.h"
#include "../parser_utils.h"
#include "../error_msg.h"

static void set_if_statement_types(Node* if_statement) {
    assert(if_statement->type == NODE_IF_STATEMENT);

    Node_if_condition* if_condition = node_unwrap_if_condition(nodes_get_child_of_type(if_statement, NODE_IF_CONDITION));
    Node* old_if_cond_child = nodes_single_child(node_wrap(if_condition));
    switch (old_if_cond_child->type) {
        case NODE_SYMBOL_UNTYPED:
            set_symbol_type(node_unwrap_symbol_untyped(old_if_cond_child));
            break;
        case NODE_OPERATOR: {
            try_set_operator_lang_type(node_unwrap_operator(old_if_cond_child));
            break;
        }
        case NODE_FUNCTION_CALL: {
            set_function_call_types(node_unwrap_function_call(old_if_cond_child));
            nodes_remove(old_if_cond_child, true);
            Node_operator* new_if_cond_child = operation_new(
                old_if_cond_child,
                node_wrap(literal_new(str_view_from_cstr("0"), TOKEN_NUM_LITERAL, node_wrap(old_if_cond_child)->pos)),
                TOKEN_NOT_EQUAL
            );
            nodes_append_child(node_wrap(if_condition), node_wrap(new_if_cond_child));
            break;
        }
        case NODE_LITERAL: {
            nodes_remove(old_if_cond_child, true);
            Node_operator* new_if_cond_child = operation_new(
                old_if_cond_child,
                node_wrap(literal_new(str_view_from_cstr("1"), TOKEN_NUM_LITERAL, old_if_cond_child->pos)),
                TOKEN_DOUBLE_EQUAL
            );
            nodes_append_child(node_wrap(if_condition), node_wrap(new_if_cond_child));
            break;
        }
        default:
            unreachable(NODE_FMT, node_print(old_if_cond_child));
    }
}

bool analysis_1(Node* start_node, int recursion_depth) {
    (void) recursion_depth;
    if (start_node->type != NODE_BLOCK) {
        return false;
    }

    nodes_foreach_child(curr_node, start_node) {
        switch (curr_node->type) {
            case NODE_ASSIGNMENT:
                set_assignment_operand_types(node_unwrap_assignment(curr_node));
                break;
            case NODE_FUNCTION_CALL:
                set_function_call_types(node_unwrap_function_call(curr_node));
                break;
            case NODE_FUNCTION_DEFINITION:
                break;
            case NODE_SYMBOL_UNTYPED:
                todo();
                set_symbol_type(node_unwrap_symbol_untyped(curr_node));
                break;
            case NODE_RETURN_STATEMENT:
                set_return_statement_types(curr_node);
                break;
            case NODE_OPERATOR:
                todo();
                break;
            case NODE_IF_STATEMENT:
                set_if_statement_types(curr_node);
                break;
            default:
                break;
        }
    }

    return false;
}
