#include "../node.h"
#include "../nodes.h"

static Node_id get_new_assignment(Node_id lhs, Node_id rhs) {
    Node_id assignment = node_new();
    nodes_at(assignment)->type = NODE_ASSIGNMENT;
    nodes_append_child(assignment, lhs);
    nodes_append_child(assignment, rhs);
    return assignment;
}

static void for_var_def_to_var_def(Node_id var_def) {
    todo();
    //Node_id var_def
    nodes_at(var_def)->type = NODE_VARIABLE_DEFINITION;

}

void for_loop_to_branch(Node_id curr_node) {
    if (nodes_at(curr_node)->type != NODE_FOR_LOOP) {
        return;
    }

    Node_id lower_bound = nodes_get_child_of_type(curr_node, NODE_FOR_LOWER_BOUND);
    Node_id upper_bound = nodes_get_child_of_type(curr_node, NODE_FOR_UPPER_BOUND);

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

    Node_id new_var_assignment = get_new_assignment(new_var_def, lower_bound);
    nodes_append_child(new_branch_block, new_var_assignment);
    nodes_replace(curr_node, new_var_assignment);
    log(LOG_DEBUG, NODE_FMT"\n", node_print(curr_node));
    log_tree(LOG_DEBUG, curr_node);
    todo();



    Node_id for_block = nodes_get_child_of_type(curr_node, NODE_BLOCK);

    Node_id lang_goto = node_new();
    nodes_at(lang_goto)->type = NODE_GOTO;

    nodes_append_child(for_block, lang_goto);
}
