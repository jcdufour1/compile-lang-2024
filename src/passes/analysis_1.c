#include "passes.h"
#include "../node.h"
#include "../nodes.h"
#include "../symbol_table.h"
#include "../parser_utils.h"
#include "../error_msg.h"

static void set_if_condition_types(Node_if_condition* if_cond) {
    Node* old_if_cond_child = if_cond->child;
    switch (old_if_cond_child->type) {
        case NODE_SYMBOL_UNTYPED:
            set_symbol_type(node_unwrap_symbol_untyped(old_if_cond_child));
            if_cond->child = node_wrap(binary_new(
                old_if_cond_child,
                node_wrap(literal_new(str_view_from_cstr("1"), TOKEN_NUM_LITERAL, old_if_cond_child->pos)),
                TOKEN_DOUBLE_EQUAL
            ));
            break;
        case NODE_OPERATOR: {
            Node_operator* operator = node_unwrap_operation(old_if_cond_child);
            if (operator->type == NODE_OP_UNARY) {
                try_set_unary_lang_type(node_unwrap_op_unary(operator));
            } else if (operator->type == NODE_OP_BINARY) {
                try_set_binary_lang_type(node_unwrap_op_binary(operator));
            } else {
                unreachable("");
            }
        }
        break;
        case NODE_FUNCTION_CALL: {
            log(LOG_DEBUG, NODE_FMT"\n", node_print(old_if_cond_child));
            set_function_call_types(node_unwrap_function_call(old_if_cond_child));
            if_cond->child = node_wrap(binary_new(
                old_if_cond_child,
                node_wrap(literal_new(str_view_from_cstr("0"), TOKEN_NUM_LITERAL, node_wrap(old_if_cond_child)->pos)),
                TOKEN_NOT_EQUAL
            ));
            break;
        }
        case NODE_LITERAL: {
            if_cond->child = node_wrap(binary_new(
                old_if_cond_child,
                node_wrap(literal_new(str_view_from_cstr("1"), TOKEN_NUM_LITERAL, old_if_cond_child->pos)),
                TOKEN_DOUBLE_EQUAL
            ));
            break;
        }
        default:
            unreachable(NODE_FMT, node_print(old_if_cond_child));
    }
}

bool analysis_1(Node* block_, int recursion_depth) {
    (void) recursion_depth;
    if (block_->type != NODE_BLOCK) {
        return false;
    }
    Node_block* block = node_unwrap_block(block_);
    Node_ptr_vec* block_children = &block->children;

    for (size_t idx = 0; idx < block_children->info.count; idx++) {
        Node* curr_node = node_ptr_vec_at(block_children, idx);

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
                set_return_statement_types(node_unwrap_return_statement(curr_node));
                break;
            case NODE_OPERATOR:
                todo();
                break;
            case NODE_IF_STATEMENT:
                set_if_condition_types(node_unwrap_if(curr_node)->condition);
                break;
            case NODE_FOR_WITH_CONDITION:
                set_if_condition_types(node_unwrap_for_with_condition(curr_node)->condition);
                break;
            default:
                break;
        }
    }

    return false;
}
