#include "passes.h"
#include "../parser_utils.h"
#include "../nodes.h"
#include "../symbol_table.h"

static Node* variable_i32_def_new(Str_view name) {
    Node* var_def = node_new();
    var_def->name = name;
    var_def->type = NODE_VARIABLE_DEFINITION;
    var_def->lang_type = str_view_from_cstr("i32");
    var_def->token_type = TOKEN_NUM_LITERAL;

    try(sym_tbl_add(var_def));
    return var_def;
}

static void flatten_operation_if_nessessary(Node* node_to_insert_before, Node* old_operation) {
    log_tree(LOG_DEBUG, old_operation);
    assert(old_operation->type == NODE_OPERATOR);
    Node* lhs = nodes_get_child(old_operation, 0);
    Node* rhs = nodes_get_child(old_operation, 1);
    nodes_remove_siblings_and_parent(lhs);
    nodes_remove_siblings_and_parent(rhs);

    if (lhs->type == NODE_OPERATOR) {
        Str_view var_name = literal_name_new();
        nodes_insert_before(node_to_insert_before, variable_i32_def_new(var_name));
        nodes_insert_before(node_to_insert_before, assignment_new(symbol_new(var_name), lhs));
        nodes_append_child(old_operation, symbol_new(var_name));
    } else if (lhs->type == NODE_FUNCTION_CALL){
        todo();
    } else {
        nodes_append_child(old_operation, lhs);
    }

    if (rhs->type == NODE_OPERATOR) {
        Str_view var_name = literal_name_new();
        nodes_insert_before(node_to_insert_before, variable_i32_def_new(var_name));
        nodes_insert_before(node_to_insert_before, assignment_new(symbol_new(var_name), rhs));
        nodes_append_child(old_operation, symbol_new(var_name));
    } else if (rhs->type == NODE_FUNCTION_CALL){
        todo();
    } else {
        nodes_append_child(old_operation, rhs);
    }
}

static void move_operator_back(Node* return_statement) {
    assert(return_statement->type == NODE_RETURN_STATEMENT);
    Node* operation = nodes_single_child(return_statement);
    assert(operation->type == NODE_OPERATOR);
    nodes_remove(operation, true);

    Str_view var_name = literal_name_new();
    nodes_insert_before(return_statement, variable_i32_def_new(var_name)); // TODO: use correct type here
    nodes_insert_before(return_statement, assignment_new(symbol_new(var_name), operation));
    nodes_append_child(return_statement, symbol_new(var_name));
}

bool flatten_operations(Node* curr_node) {
    if (curr_node->type != NODE_BLOCK) {
        return false;
    }

    Node* assign_or_var_def = nodes_get_local_rightmost(curr_node->left_child);
    while (assign_or_var_def) {
        bool advance_to_prev = true;
        if (assign_or_var_def->type == NODE_ASSIGNMENT) {
            nodes_foreach_child(potential_op, assign_or_var_def) {
                if (potential_op->type == NODE_OPERATOR) {
                    flatten_operation_if_nessessary(assign_or_var_def, potential_op);
                }
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
                move_operator_back(assign_or_var_def);
            }
        }

        if (advance_to_prev) {
            assign_or_var_def = assign_or_var_def->prev;
        }
    }

    return false;
}

