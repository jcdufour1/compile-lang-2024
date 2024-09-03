#include "../symbol_table.h"
#include "../parser_utils.h"
#include "../nodes.h"

static Node_id assignment_new(Node_id lhs, Node_id rhs) {
    nodes_remove_siblings_and_parent(lhs);
    nodes_remove_siblings_and_parent(rhs);

    Node_id assignment = node_new();
    nodes_at(assignment)->type = NODE_ASSIGNMENT;
    nodes_append_child(assignment, lhs);
    nodes_append_child(assignment, rhs);
    return assignment;
}

static Node_id operation_new(Node_id lhs, Node_id rhs, TOKEN_TYPE operation_type) {
    // TODO: check if lhs or rhs were already appended to the tree
    Node_id assignment = node_new();
    nodes_at(assignment)->type = NODE_OPERATOR;
    nodes_at(assignment)->token_type = operation_type;
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
    Str_view label_name_if_false,
    Node_id upper_bound
) {
    if (nodes_at(nodes_single_child(upper_bound))->type != NODE_LITERAL) {
        todo();
    }

    Str_view upper_bound_str = nodes_at(nodes_single_child(upper_bound))->str_data;
    Node_id less_than = node_new();
    nodes_at(less_than)->type = NODE_OPERATOR;
    nodes_at(less_than)->token_type = TOKEN_LESS_THAN;
    nodes_append_child(less_than, symbol_new(symbol_name_to_check));
    nodes_append_child(less_than, literal_new(upper_bound_str));

    Node_id cond_goto = node_new();
    nodes_at(cond_goto)->type = NODE_COND_GOTO;
    nodes_append_child(cond_goto, less_than);
    nodes_append_child(cond_goto, symbol_new(label_name_if_true));
    nodes_append_child(cond_goto, symbol_new(label_name_if_false));
    return cond_goto;
}

static Node_id get_for_loop_cond_var_assign(Str_view sym_name) {
    Node_id literal = literal_new(str_view_from_cstr("1"));
    Node_id operation = operation_new(symbol_new(sym_name), literal, TOKEN_SINGLE_PLUS);
    return assignment_new(symbol_new(sym_name), operation);
}

static void for_loop_to_branch(Node_id for_loop) {
    Node_id lower_bound = nodes_get_child_of_type(for_loop, NODE_FOR_LOWER_BOUND);
    Node_id upper_bound = nodes_get_child_of_type(for_loop, NODE_FOR_UPPER_BOUND);
    Node_id for_block = nodes_get_child_of_type(for_loop, NODE_BLOCK);

    Node_id new_branch_block = node_new();
    nodes_at(new_branch_block)->type = NODE_BLOCK;
    assert(node_is_null(nodes_parent(new_branch_block)));

    Node_id symbol_lhs_assign;
    Node_id regular_var_def;
    {
        Node_id for_var_def;
        for_var_def = nodes_get_child_of_type(for_loop, NODE_FOR_VARIABLE_DEF);
        regular_var_def = nodes_get_child_of_type(for_var_def, NODE_VARIABLE_DEFINITION);
        nodes_remove(regular_var_def, false);
        symbol_lhs_assign = symbol_new(nodes_at(regular_var_def)->name);
        nodes_remove_siblings(for_var_def);
    }

    Node_id assignment_to_inc_cond_var = get_for_loop_cond_var_assign(nodes_at(regular_var_def)->name);
    assert(node_is_null(nodes_parent(new_branch_block)));
    nodes_remove_siblings_and_parent(lower_bound);
    nodes_remove_siblings_and_parent(upper_bound);
    nodes_remove_siblings_and_parent(for_block);

    // initial assignment
    Node_id new_var_assign = assignment_new(symbol_lhs_assign, nodes_at(lower_bound)->left_child);

    Node_id jmp_to_check_cond_label = goto_new(str_view_from_cstr("for_start"));
    Node_id check_cond_label = label_new(str_view_from_cstr("for_start"));
    Node_id after_check_label = label_new(str_view_from_cstr("for_after_check"));
    Node_id after_for_loop_label = label_new(str_view_from_cstr("for_after"));
    Node_id check_cond_jmp = jmp_if_less_than_new(
        nodes_at(regular_var_def)->name, 
        nodes_at(after_check_label)->name, 
        nodes_at(after_for_loop_label)->name,
        upper_bound
    );

    nodes_append_child(new_branch_block, regular_var_def);
    nodes_append_child(new_branch_block, new_var_assign);
    nodes_append_child(new_branch_block, jmp_to_check_cond_label);
    nodes_append_child(new_branch_block, check_cond_label);
    nodes_append_child(new_branch_block, check_cond_jmp);
    nodes_append_child(new_branch_block, after_check_label);
    nodes_extend_children(new_branch_block, nodes_left_child(for_block)); // this is the problem
    nodes_append_child(new_branch_block, assignment_to_inc_cond_var);
    nodes_append_child(new_branch_block, goto_new(nodes_at(check_cond_label)->name));

    nodes_append_child(new_branch_block, after_for_loop_label);

    nodes_replace(for_loop, new_branch_block);
}

static void if_statement_to_branch(Node_id curr_node) {
    todo();
}

bool for_and_if_to_branch(Node_id curr_node) {
    switch (nodes_at(curr_node)->type) {
        case NODE_FOR_LOOP:
            for_loop_to_branch(curr_node);
            return false;
        case NODE_IF_STATEMENT:
            if_statement_to_branch(curr_node);
            return false;
        default:
            return false;
    }
}

