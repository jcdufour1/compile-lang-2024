#include "passes.h"
#include "../parser_utils.h"
#include "../nodes.h"
#include "../symbol_table.h"

// returns operand or operand symbol
static Node* flatten_operation_operand(Node* node_to_insert_before, Node* operand) {
    if (operand->type == NODE_OPERATOR) {
        Node* operator_sym = node_new(operand->pos, NODE_OPERATOR_RETURN_VALUE_SYM);
        nodes_insert_before(node_to_insert_before, operand);
        node_unwrap_operator_rtn_val_sym(operator_sym)->node_src = operand;
        return operator_sym;
    } else if (operand->type == NODE_FUNCTION_CALL) {
        Node_function_rtn_val_sym* fun_sym = node_unwrap_function_rtn_val_sym(node_new(operand->pos, NODE_FUNCTION_RETURN_VALUE_SYM));
        nodes_insert_before(node_to_insert_before, operand);
        fun_sym->node_src = operand;
        return node_wrap(fun_sym);
    } else {
        return operand;
    }
}

static void flatten_operation_if_nessessary(Node* node_to_insert_before, Node* old_operation) {
    assert(old_operation->type == NODE_OPERATOR);
    Node* lhs = nodes_get_child(old_operation, 0);
    Node* rhs = nodes_get_child(old_operation, 1);
    nodes_remove(lhs, true);
    nodes_remove(rhs, true);
    nodes_append_child(old_operation, flatten_operation_operand(node_to_insert_before, lhs));
    nodes_append_child(old_operation, flatten_operation_operand(node_to_insert_before, rhs));
}

// operator symbol is returned
static Node_operator_rtn_val_sym* move_operator_back(Node* node_to_insert_before, Node_operator* operation) {
    nodes_remove(node_wrap(operation), true);

    Node_operator_rtn_val_sym* operator_sym = node_unwrap_operator_rtn_val_sym(node_new(node_wrap(operation)->pos, NODE_OPERATOR_RETURN_VALUE_SYM));
    operator_sym->node_src = node_wrap(operation);
    try(try_set_operator_lang_type(operation));
    operator_sym->lang_type = operation->lang_type;
    assert(operator_sym->lang_type.str.count > 0);
    nodes_insert_before(node_to_insert_before, node_wrap(operation));
    return operator_sym;
}

bool flatten_operations(Node* curr_node) {
    if (curr_node->type != NODE_BLOCK) {
        return false;
    }

    Node* assign_or_var_def = nodes_get_local_rightmost(get_left_child(curr_node));
    while (assign_or_var_def) {
        bool advance_to_prev = true;
        if (assign_or_var_def->type == NODE_OPERATOR) {
            flatten_operation_if_nessessary(assign_or_var_def, assign_or_var_def);
        } else if (assign_or_var_def->type == NODE_ASSIGNMENT) {
            Node* rhs = nodes_get_child(assign_or_var_def, 1);
            if (rhs->type == NODE_OPERATOR) {
                nodes_append_child(assign_or_var_def, node_wrap(move_operator_back(assign_or_var_def, node_unwrap_operator(rhs))));
            }
        } else if (assign_or_var_def->type == NODE_VARIABLE_DEFINITION) {
            if (assign_or_var_def->prev && assign_or_var_def->prev->type != NODE_VARIABLE_DEFINITION) {
                Node* init_prev = assign_or_var_def->prev;
                nodes_move_back_one(assign_or_var_def);
                assign_or_var_def = init_prev;
                advance_to_prev = false;
            }
        } else if (assign_or_var_def->type == NODE_RETURN_STATEMENT) {
            if (nodes_single_child(assign_or_var_def)->type == NODE_OPERATOR) {
                nodes_append_child(
                    assign_or_var_def,
                    node_wrap(move_operator_back(assign_or_var_def, node_unwrap_operator(nodes_single_child(assign_or_var_def))))
                );
            }
        }

        if (advance_to_prev) {
            assign_or_var_def = assign_or_var_def->prev;
        }
    }

    return false;
}

