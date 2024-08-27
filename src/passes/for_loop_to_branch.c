#include "../node.h"
#include "../nodes.h"

static Node_id assignment_new(Node_id lhs, Node_id rhs) {
    Node_id assignment = node_new();
    nodes_at(assignment)->type = NODE_ASSIGNMENT;
    nodes_append_child(assignment, lhs);
    nodes_append_child(assignment, rhs);
    return assignment;
}

static Node_id for_loop_start_label_new(Str_view label_name) {
    Node_id label = node_new();
    nodes_at(label)->type = NODE_LABEL;
    nodes_at(label)->str_data = label_name;
    return label;
}

static Node_id goto_new(Str_view name_label_to_jmp_to) {
    Node_id lang_goto = node_new();
    nodes_at(lang_goto)->type = NODE_GOTO;
    nodes_at(lang_goto)->str_data = name_label_to_jmp_to;
    return lang_goto;
}

void for_loop_to_branch(Node_id curr_node) {
    if (nodes_at(curr_node)->type != NODE_FOR_LOOP) {
        return;
    }

    Node_id lower_bound = nodes_get_child_of_type(curr_node, NODE_FOR_LOWER_BOUND);
    Node_id upper_bound = nodes_get_child_of_type(curr_node, NODE_FOR_UPPER_BOUND);
    Node_id for_block = nodes_get_child_of_type(curr_node, NODE_BLOCK);

    Node_id new_branch_block = node_new();
    nodes_at(new_branch_block)->type = NODE_BLOCK;

    Node_id new_var_def;
    {
        Node_id old_var_def;
        new_var_def = nodes_get_child_of_type(curr_node, NODE_FOR_VARIABLE_DEF);
        old_var_def = nodes_get_child_of_type(new_var_def, NODE_VARIABLE_DEFINITION);
        nodes_replace(new_var_def, old_var_def);
    }
    nodes_remove_siblings(new_var_def);
    nodes_remove_siblings(lower_bound);
    nodes_remove_siblings(upper_bound);
    nodes_remove_siblings(for_block);

    Node_id new_var_assignment = assignment_new(new_var_def, nodes_at(lower_bound)->left_child);
    nodes_append_child(new_branch_block, new_var_assignment);

    Node_id start_label = for_loop_start_label_new(str_view_from_cstr("for_start"));
    nodes_append_child(new_branch_block, start_label);
    nodes_extend_children(new_branch_block, for_block);
    nodes_append_child(new_branch_block, goto_new(nodes_at(start_label)->str_data));

    nodes_replace(curr_node, new_branch_block);

    log_tree(LOG_DEBUG, curr_node);
    todo();
}
