#include "passes.h"
#include "../node.h"
#include "../nodes.h"
#include "../symbol_table.h"

static void do_assignment(Node* assignment) {
    Node* lhs = nodes_get_child(assignment, 0);
    Node* rhs = nodes_get_child(assignment, 1);
    if (rhs->type == NODE_STRUCT_LITERAL) {
        if (lhs->type == NODE_VARIABLE_DEFINITION) {
            rhs->lang_type = lhs->lang_type;
        } else if (lhs->type == NODE_SYMBOL) {
            Node* sym_def;
            try(sym_tbl_lookup(&sym_def, lhs->name));
            rhs->lang_type = sym_def->lang_type;
        } else {
            unreachable("");
        }
    }
}

static void do_function_call(Node* fun_call) {
    Node* fun_def;
    try(sym_tbl_lookup(&fun_def, fun_call->name));
    Node* params = nodes_get_child_of_type(fun_def, NODE_FUNCTION_PARAMETERS);
    size_t idx = 0;
    nodes_foreach_child(argument, fun_call) {
        Node* corresponding_param = nodes_get_child(params, idx);
        switch (corresponding_param->pointer_depth) {
            case 0:
                break;
            case 1:
                argument->pointer_depth = 1;
                break;
            default:
                todo();
        }

        idx++;
    }
}

bool analysis_1(Node* start_node) {
    if (start_node->type != NODE_BLOCK) {
        return false;
    }

    Node* curr_node = nodes_get_local_rightmost(start_node->left_child);
    while (curr_node) {
        bool go_to_prev = true;
        switch (curr_node->type) {
            case NODE_ASSIGNMENT:
                do_assignment(curr_node);
                break;
            case NODE_FUNCTION_CALL:
                do_function_call(curr_node);
                break;
            default:
                break;
        }

        if (go_to_prev) {
            curr_node = curr_node->prev;
        }
    }

    return false;
}
