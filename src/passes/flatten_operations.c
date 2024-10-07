#include "passes.h"
#include "../parser_utils.h"
#include "../nodes.h"
#include "../symbol_table.h"

// returns operand or operand symbol
static Node* flatten_operation_operand(Node* node_to_insert_before, Node* operand) {
    if (operand->type == NODE_OPERATOR) {
        Node* operator_sym = node_new(operand->pos, NODE_LLVM_REGISTER_SYM);
        nodes_insert_before(node_to_insert_before, operand);
        node_unwrap_llvm_register_sym(operator_sym)->node_src = operand;
        return operator_sym;
    } else if (operand->type == NODE_FUNCTION_CALL) {
        Node_llvm_register_sym* fun_sym = node_unwrap_llvm_register_sym(node_new(operand->pos, NODE_LLVM_REGISTER_SYM));
        nodes_insert_before(node_to_insert_before, operand);
        fun_sym->node_src = operand;
        return node_wrap(fun_sym);
    } else {
        return operand;
    }
}

static void flatten_operation_if_nessessary(Node* node_to_insert_before, Node_operator* old_operation) {
    nodes_remove(old_operation->lhs, true);
    nodes_remove(old_operation->rhs, true);
    old_operation->lhs = flatten_operation_operand(node_to_insert_before, old_operation->lhs);
    old_operation->rhs = flatten_operation_operand(node_to_insert_before, old_operation->rhs);
}

// operator symbol is returned
static Node_llvm_register_sym* move_operator_back(Node* node_to_insert_before, Node_operator* operation) {
    nodes_remove(node_wrap(operation), true);

    Node_llvm_register_sym* operator_sym = node_unwrap_llvm_register_sym(node_new(node_wrap(operation)->pos, NODE_LLVM_REGISTER_SYM));
    operator_sym->node_src = node_wrap(operation);
    try(try_set_operator_lang_type(operation));
    operator_sym->lang_type = operation->lang_type;
    assert(operator_sym->lang_type.str.count > 0);
    nodes_insert_before(node_to_insert_before, node_wrap(operation));
    return operator_sym;
}

bool flatten_operations(Node* curr_node, int recursion_depth) {
    (void) recursion_depth;
    if (curr_node->type != NODE_BLOCK) {
        return false;
    }

    Node* assign_or_var_def = nodes_get_local_rightmost(get_left_child(curr_node));
    while (assign_or_var_def) {
        bool advance_to_prev = true;
        if (assign_or_var_def->type == NODE_OPERATOR) {
            flatten_operation_if_nessessary(assign_or_var_def, node_unwrap_operator(assign_or_var_def));
        } else if (assign_or_var_def->type == NODE_ASSIGNMENT) {
            Node* rhs = node_unwrap_assignment(assign_or_var_def)->rhs;
            if (rhs->type == NODE_OPERATOR) {
                node_unwrap_assignment(assign_or_var_def)->rhs = node_wrap(
                    move_operator_back(assign_or_var_def, node_unwrap_operator(rhs))
                );
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

