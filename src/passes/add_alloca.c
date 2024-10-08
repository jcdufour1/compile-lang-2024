#include "passes.h"
#include "../node.h"
#include "../nodes.h"
#include "../parser_utils.h"

static Node_alloca* alloca_new(Node_variable_def* var_def) {
    Node_alloca* alloca = node_unwrap_alloca(node_new(node_wrap(var_def)->pos, NODE_ALLOCA));
    alloca->name = var_def->name;
    alloca->lang_type = var_def->lang_type;
    var_def->storage_location = node_wrap(alloca);
    return alloca;
}

static void do_function_definition(Node_function_definition* fun_def) {
    nodes_foreach_child(param, fun_def->declaration->parameters) {
        if (is_corresponding_to_a_struct(param)) {
            node_unwrap_variable_def(param)->storage_location = param;
            continue;
        }
        Node_alloca* alloca = alloca_new(node_unwrap_variable_def(param));
        nodes_insert_before(fun_def->body->child, node_wrap(alloca));
    }
}


static void insert_alloca(Node* start_of_block, Node_variable_def* var_def) {
    Node_alloca* new_alloca = alloca_new(var_def);

    nodes_foreach_from_curr(curr, start_of_block) {
        if (curr->type != NODE_VARIABLE_DEFINITION) {
            if (curr->prev) {
                nodes_insert_after(curr->prev, node_wrap(new_alloca));
                return;
            }
            nodes_insert_before(curr, node_wrap(new_alloca));
            return;
        }
    }


    unreachable("");
}

static void do_assignment(Node* start_of_block, Node_assignment* assignment) {
    if (assignment->lhs->type == NODE_VARIABLE_DEFINITION) {
        insert_alloca(start_of_block, node_unwrap_variable_def(assignment->lhs));
    }
}

bool add_alloca(Node* start_start_node, int recursion_depth) {
    (void) recursion_depth;
    if (start_start_node->type != NODE_BLOCK) {
        return false;
    }
    if (!get_left_child(start_start_node)) {
        return false;
    }

    Node* curr_node = nodes_get_local_rightmost(get_left_child(start_start_node));
    Node* start_of_block = get_left_child(start_start_node);
    while (curr_node) {
        bool go_to_prev = true;
        switch (curr_node->type) {
            case NODE_VARIABLE_DEFINITION:
                insert_alloca(start_of_block, node_unwrap_variable_def(curr_node));
                break;
            case NODE_FUNCTION_DEFINITION:
                do_function_definition(node_unwrap_function_definition(curr_node));
                break;
            case NODE_ASSIGNMENT:
                do_assignment(start_of_block, node_unwrap_assignment(curr_node));
            default:
                break;
        }

        if (go_to_prev) {
            curr_node = curr_node->prev;
        }
    }

    return false;
}
