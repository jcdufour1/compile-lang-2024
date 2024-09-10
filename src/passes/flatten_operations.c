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

    sym_tbl_add(var_def);
    return var_def;
}

static void flatten_operation_if_nessessary(Node* node_to_insert_before, Node* old_operation) {
    log(LOG_DEBUG, "DOING_THING\n");
    log_tree(LOG_DEBUG, old_operation);
    assert(old_operation->type == NODE_OPERATOR);
    Node* lhs = nodes_get_child(old_operation, 0);
    Node* rhs = nodes_get_child(old_operation, 1);
    nodes_remove_siblings_and_parent(lhs);
    nodes_remove_siblings_and_parent(rhs);

    if (lhs->type == NODE_OPERATOR) {

        log(LOG_DEBUG, "REALLY DOING THING\n");
        Str_view var_name = literal_name_new();
        Node* var_def = variable_i32_def_new(var_name);
        node_printf(var_def);
        nodes_insert_before(node_to_insert_before, var_def);

        Node* child_assignment = assignment_new(symbol_new(var_name), lhs);
        nodes_insert_before(node_to_insert_before, child_assignment);

        nodes_append_child(old_operation, symbol_new(var_name));

        log_tree(LOG_DEBUG, node_to_insert_before->parent);
    } else if (lhs->type == NODE_FUNCTION_CALL){
        todo();
    } else {
        nodes_append_child(old_operation, lhs);
    }

    if (rhs->type == NODE_OPERATOR) {
        todo();
    } else if (rhs->type == NODE_FUNCTION_CALL){
        todo();
    } else {
        nodes_append_child(old_operation, rhs);
    }
}

bool flatten_operations(Node* curr_node) {
    if (curr_node->type != NODE_BLOCK) {
        return false;
    }

    log_tree(LOG_DEBUG, curr_node);
    nodes_foreach_child_rev(potential_assign, curr_node) {
        log_tree(LOG_DEBUG, potential_assign);
        if (potential_assign->type == NODE_ASSIGNMENT) {
            nodes_foreach_child(potential_op, potential_assign) {
                if (potential_op->type == NODE_OPERATOR) {
                    flatten_operation_if_nessessary(potential_assign, potential_op);
                }
            }
        } else if (potential_assign->type == NODE_VARIABLE_DEFINITION) {
            Node* var_def = potential_assign;
            while (var_def->prev && var_def->prev->type != NODE_VARIABLE_DEFINITION) {
                nodes_move_back_one(var_def);
            }
        }
    }

    return false;
}
