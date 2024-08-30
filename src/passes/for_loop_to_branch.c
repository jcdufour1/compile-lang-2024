#include "../node.h"
#include "../nodes.h"
#include "../symbol_table.h"
#include "../parser_utils.h"

static Node_id assignment_new(Node_id lhs, Node_id rhs) {
    Node_id assignment = node_new();
    nodes_at(assignment)->type = NODE_ASSIGNMENT;
    nodes_append_child(assignment, lhs);
    nodes_append_child(assignment, rhs);
    return assignment;
}

static Node_id label_new(Str_view label_name) {
    Node_id label = node_new();
    nodes_at(label)->type = NODE_LABEL;
    nodes_at(label)->name = label_name;
    sym_tbl_add(label);
    return label;
}

static Node_id goto_new(Str_view name_label_to_jmp_to) {
    Node_id lang_goto = node_new();
    nodes_at(lang_goto)->type = NODE_GOTO;
    nodes_at(lang_goto)->name = name_label_to_jmp_to;
    return lang_goto;
}

static Node_id symbol_new(Str_view symbol_name) {
    assert(symbol_name.count > 0);

    Node_id symbol = node_new();
    nodes_at(symbol)->type = NODE_SYMBOL;
    nodes_at(symbol)->name = symbol_name;
    return symbol;
}

static Node_id literal_new(Str_view value) {
    Node_id symbol = node_new();
    nodes_at(symbol)->type = NODE_LITERAL;
    nodes_at(symbol)->name = literal_name_new();
    nodes_at(symbol)->str_data = value;
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
    nodes_append_child(less_than, symbol_new(symbol_name_to_check));
    nodes_append_child(less_than, literal_new(str_view_from_cstr("2")));

    Node_id cond_goto = node_new();
    nodes_at(cond_goto)->type = NODE_COND_GOTO;
    nodes_append_child(cond_goto, less_than);
    nodes_append_child(cond_goto, symbol_new(label_name_if_true));
    nodes_append_child(cond_goto, symbol_new(label_name_if_false));
    return cond_goto;
}

void for_loop_to_branch(Node_id for_loop) {
    if (nodes_at(for_loop)->type != NODE_FOR_LOOP) {
        return;
    }

    Node_id lower_bound = nodes_get_child_of_type(for_loop, NODE_FOR_LOWER_BOUND);
    Node_id upper_bound = nodes_get_child_of_type(for_loop, NODE_FOR_UPPER_BOUND);
    Node_id for_block = nodes_get_child_of_type(for_loop, NODE_BLOCK);

    Node_id new_branch_block = node_new();
    nodes_at(new_branch_block)->type = NODE_BLOCK;
    assert(nodes_at(new_branch_block)->parent == NODE_IDX_NULL);

    log_tree(LOG_DEBUG, 0);
    Node_id symbol_lhs_assign;
    Node_id old_var_def;
    {
        Node_id new_var_def;
        new_var_def = nodes_get_child_of_type(for_loop, NODE_FOR_VARIABLE_DEF);
        old_var_def = nodes_get_child_of_type(new_var_def, NODE_VARIABLE_DEFINITION);
        nodes_remove(old_var_def);
        log_tree(LOG_DEBUG, 0);
        nodes_insert_before(for_loop, old_var_def);
        log_tree(LOG_DEBUG, 0);
        node_printf(for_loop);
        node_printf(old_var_def);
        symbol_lhs_assign = symbol_new(nodes_at(old_var_def)->name);
        nodes_remove_siblings(new_var_def);
    }
    assert(nodes_at(new_branch_block)->parent == NODE_IDX_NULL);
    log_tree(LOG_DEBUG, 0);
    nodes_remove_siblings(lower_bound);
    nodes_remove_siblings(upper_bound);
    nodes_remove_siblings(for_block);
    log_tree(LOG_DEBUG, 0);
    assert(nodes_at(new_branch_block)->parent == NODE_IDX_NULL);

    Node_id new_var_assign = assignment_new(symbol_lhs_assign, nodes_at(lower_bound)->left_child);
    nodes_append_child(new_branch_block, new_var_assign);

    log_tree(LOG_DEBUG, 0);
    Node_id check_cond_label = label_new(str_view_from_cstr("for_start"));
    Node_id after_check_label = label_new(str_view_from_cstr("for_after_check"));
    Node_id after_for_loop_label = label_new(str_view_from_cstr("for_after"));
    log_tree(LOG_DEBUG, 0);
    Node_id check_cond_jmp = jmp_if_less_than_new(
        nodes_at(old_var_def)->name, 
        nodes_at(after_check_label)->name, 
        nodes_at(after_for_loop_label)->name
    );
    log_tree(LOG_DEBUG, 0);

    nodes_append_child(new_branch_block, check_cond_label);
    nodes_append_child(new_branch_block, check_cond_jmp);
    nodes_append_child(new_branch_block, after_check_label);
    nodes_extend_children(new_branch_block, nodes_single_child(for_block));
    nodes_append_child(new_branch_block, goto_new(nodes_at(check_cond_label)->name));
    log_tree(LOG_DEBUG, 0);

    nodes_append_child(new_branch_block, after_for_loop_label);
    log_tree(LOG_DEBUG, 0);

    nodes_replace(for_loop, new_branch_block);
    log_tree(LOG_DEBUG, 0);
}
