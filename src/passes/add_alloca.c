#include "passes.h"
#include "../node.h"
#include "../nodes.h"
#include "../parser_utils.h"

static Node* alloca_new(Node* var_def) {
    Node* alloca = node_new();
    alloca->type = NODE_ALLOCA;
    alloca->name = var_def->name;
    var_def->storage_location = alloca;
    return alloca;
}

static void do_function_definition(Node* fun_def) {
    assert(fun_def->type == NODE_FUNCTION_DEFINITION);
    Node* fun_params = nodes_get_child_of_type(fun_def, NODE_FUNCTION_PARAMETERS);
    Node* fun_block = nodes_get_child_of_type(fun_def, NODE_BLOCK);

    nodes_foreach_child(param, fun_params) {
        if (is_corresponding_to_a_struct(param)) {
            param->storage_location = param;
            continue;
        }
        Node* alloca = alloca_new(param);
        nodes_insert_before(fun_block->left_child, alloca);
    }
}


static void insert_alloca(Node* start_of_block, Node* var_def) {
    Node* new_alloca = alloca_new(var_def);

    nodes_foreach_from_curr(curr, start_of_block) {
        if (curr->type != NODE_VARIABLE_DEFINITION) {
            if (curr->prev) {
                nodes_insert_after(curr->prev, new_alloca);
                return;
            }
            nodes_insert_before(curr, new_alloca);
            return;
        }
    }


    unreachable("");
}

static void do_assignment(Node* start_of_block, Node* assignment) {
    Node* lhs = nodes_get_child(assignment, 0);
    if (lhs->type == NODE_VARIABLE_DEFINITION) {
        insert_alloca(start_of_block, lhs);
    }
}

bool add_alloca(Node* start_start_node) {
    if (!start_start_node->left_child) {
        return false;
    }
    if (start_start_node->type != NODE_BLOCK) {
        return false;
    }

    Node* curr_node = nodes_get_local_rightmost(start_start_node->left_child);
    Node* start_of_block = start_start_node->left_child;
    while (curr_node) {
        bool go_to_prev = true;
        switch (curr_node->type) {
            case NODE_VARIABLE_DEFINITION:
                insert_alloca(start_of_block, curr_node);
                break;
            case NODE_FUNCTION_DEFINITION:
                do_function_definition(curr_node);
                break;
            case NODE_ASSIGNMENT:
                do_assignment(start_of_block, curr_node);
            default:
                break;
        }

        if (go_to_prev) {
            curr_node = curr_node->prev;
        }
    }

    return false;
}
