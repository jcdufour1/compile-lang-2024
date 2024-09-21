#include "passes.h"
#include "../node.h"
#include "../nodes.h"
#include "../symbol_table.h"

static void do_assignment(Node* curr_node) {
    Node* lhs = nodes_get_child(curr_node, 0);
    Node* rhs = nodes_get_child(curr_node, 1);
    if (rhs->type == NODE_STRUCT_LITERAL) {
        if (lhs->type != NODE_SYMBOL) {
            todo();
        }
        Node* sym_def;
        try(sym_tbl_lookup(&sym_def, lhs->name));
        rhs->lang_type = sym_def->lang_type;
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
            default:
                break;
        }

        if (go_to_prev) {
            curr_node = curr_node->prev;
        }
    }

    return false;
}
