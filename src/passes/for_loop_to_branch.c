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
    nodes_at(label)->name = label_name;
    return label;
}

static Node_id goto_new(Str_view name_label_to_jmp_to) {
    Node_id lang_goto = node_new();
    nodes_at(lang_goto)->type = NODE_GOTO;
    nodes_at(lang_goto)->name = name_label_to_jmp_to;
    return lang_goto;
}

static Node_id symbol_new(Str_view symbol_name) {
    Node_id symbol = node_new();
    nodes_at(symbol)->type = NODE_SYMBOL;
    nodes_at(symbol)->name = symbol_name;
    return symbol;
}

static Node_id jmp_if_less_than_new(
    Str_view symbol_name_to_check,
    Str_view label_name_if_true,
    Str_view label_name_if_false
) {
    Node_id less_than = node_new();
    nodes_at(less_than)->type = NODE_OPERATOR;
    nodes_at(less_than)->token_type = TOKEN_LESS_THAN;
    nodes_append_child(less_than, symbol_new(label_name_if_true));
    nodes_append_child(less_than, symbol_new(label_name_if_false));

    Node_id cond_goto = node_new();
    nodes_at(cond_goto)->type = NODE_COND_GOTO;
    nodes_at(cond_goto)->name = symbol_name_to_check;
    nodes_append_child(cond_goto, less_than);
    return cond_goto;
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

    Node_id check_cond_label = for_loop_start_label_new(str_view_from_cstr("for_start"));
    Node_id after_check_label = for_loop_start_label_new(str_view_from_cstr("for_after_check"));
    Node_id after_for_loop_label = for_loop_start_label_new(str_view_from_cstr("for_after"));
    Node_id check_cond_jmp = jmp_if_less_than_new(
        nodes_at(new_var_def)->name, 
        nodes_at(after_check_label)->name, 
        nodes_at(after_for_loop_label)->name
    );

    nodes_append_child(new_branch_block, check_cond_label);
    nodes_append_child(new_branch_block, check_cond_jmp);
    nodes_append_child(new_branch_block, after_check_label);
    nodes_extend_children(new_branch_block, nodes_single_child(for_block));
    nodes_append_child(new_branch_block, goto_new(nodes_at(check_cond_label)->name));

    nodes_append_child(new_branch_block, after_for_loop_label);

    log_tree(LOG_DEBUG, new_branch_block);
    todo();

    nodes_replace(curr_node, new_branch_block);
}
