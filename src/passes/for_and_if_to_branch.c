#include "../symbol_table.h"
#include "../parser_utils.h"
#include "../nodes.h"

static Node* operation_new(Node* lhs, Node* rhs, TOKEN_TYPE operation_type) {
    // TODO: check if lhs or rhs were already appended to the tree
    Node* assignment = node_new();
    assignment->type = NODE_OPERATOR;
    assignment->token_type = operation_type;
    nodes_append_child(assignment, lhs);
    nodes_append_child(assignment, rhs);
    return assignment;
}

static Node* label_new(Str_view label_name) {
    Node* label = node_new();
    label->type = NODE_LABEL;
    label->name = label_name;
    sym_tbl_add(label);
    return label;
}

static Node* goto_new(Str_view name_label_to_jmp_to) {
    Node* lang_goto = node_new();
    lang_goto->type = NODE_GOTO;
    lang_goto->name = name_label_to_jmp_to;
    return lang_goto;
}

static Node* cond_goto_new(
    Str_view symbol_name_to_check,
    Str_view label_name_if_true,
    Str_view label_name_if_false,
    TOKEN_TYPE operator_type,
    const Node* upper_bound
) {
    Node* upper_bound_copy = node_clone(upper_bound);

    Node* operator = node_new();
    operator->type = NODE_OPERATOR;
    operator->token_type = operator_type;
    nodes_append_child(operator, symbol_new(symbol_name_to_check));
    nodes_append_child(operator, upper_bound_copy);

    Node* cond_goto = node_new();
    cond_goto->type = NODE_COND_GOTO;
    nodes_append_child(cond_goto, operator);
    nodes_append_child(cond_goto, symbol_new(label_name_if_true));
    nodes_append_child(cond_goto, symbol_new(label_name_if_false));
    return cond_goto;
}

static Node* get_for_loop_cond_var_assign(Str_view sym_name) {
    Node* literal = literal_new(str_view_from_cstr("1"));
    Node* operation = operation_new(symbol_new(sym_name), literal, TOKEN_SINGLE_PLUS);
    return assignment_new(symbol_new(sym_name), operation);
}

static void for_loop_to_branch(Node* for_loop) {
    Node* lower_bound = nodes_get_child_of_type(for_loop, NODE_FOR_LOWER_BOUND);
    Node* upper_bound = nodes_get_child_of_type(for_loop, NODE_FOR_UPPER_BOUND);
    Node* for_block = nodes_get_child_of_type(for_loop, NODE_BLOCK);

    Node* new_branch_block = node_new();
    new_branch_block->type = NODE_BLOCK;
    assert(!new_branch_block->parent);

    Node* symbol_lhs_assign;
    Node* regular_var_def;
    {
        Node* new_var_def = nodes_get_child_of_type(for_loop, NODE_FOR_VARIABLE_DEF);
        regular_var_def = nodes_get_child_of_type(new_var_def, NODE_VARIABLE_DEFINITION);
        nodes_remove(regular_var_def, false);
        symbol_lhs_assign = symbol_new(regular_var_def->name);
        nodes_remove_siblings(new_var_def);
    }

    Node* assignment_to_inc_cond_var = get_for_loop_cond_var_assign(regular_var_def->name);
    assert(!new_branch_block->parent);
    nodes_remove_siblings_and_parent(lower_bound);
    Node* lower_bound_child = nodes_single_child(lower_bound);
    nodes_remove_siblings_and_parent(lower_bound_child);
    nodes_remove_siblings_and_parent(upper_bound);
    nodes_remove_siblings_and_parent(for_block);

    // initial assignment
    Node* new_var_assign = assignment_new(symbol_lhs_assign, lower_bound_child);

    Node* check_cond_label = label_new(literal_name_new());
    Node* jmp_to_check_cond_label = goto_new(check_cond_label->name);
    Node* after_check_label = label_new(literal_name_new());
    Node* after_for_loop_label = label_new(literal_name_new());
    Node* check_cond_jmp = cond_goto_new(
        regular_var_def->name, 
        after_check_label->name, 
        after_for_loop_label->name,
        TOKEN_LESS_THAN,
        nodes_single_child(upper_bound)
    );

    nodes_append_child(new_branch_block, regular_var_def);
    nodes_append_child(new_branch_block, new_var_assign);
    nodes_append_child(new_branch_block, jmp_to_check_cond_label);
    nodes_append_child(new_branch_block, check_cond_label);
    nodes_append_child(new_branch_block, check_cond_jmp);
    nodes_append_child(new_branch_block, after_check_label);
    nodes_extend_children(new_branch_block, for_block->left_child);
    nodes_append_child(new_branch_block, assignment_to_inc_cond_var);
    nodes_append_child(new_branch_block, goto_new(check_cond_label->name));
    nodes_append_child(new_branch_block, after_for_loop_label);

    nodes_replace(for_loop, new_branch_block);
}

static void if_statement_to_branch(Node* curr_node) {
    log_tree(LOG_DEBUG, curr_node);

    Node* condition = nodes_get_child_of_type(curr_node, NODE_IF_CONDITION);
    Node* block = nodes_get_child_of_type(curr_node, NODE_BLOCK);

    Node* operation = nodes_get_child_of_type(condition, NODE_OPERATOR);
    Node* symbol_to_check = nodes_get_child(operation, 0);
    assert(symbol_to_check->type == NODE_SYMBOL);
    Node* upper_bound = nodes_get_child(operation, 1);
    assert(upper_bound->type == NODE_LITERAL);
    assert(upper_bound->token_type == TOKEN_NUM_LITERAL);
    nodes_remove_siblings_and_parent(upper_bound);

    Node* if_true = label_new(literal_name_new());
    Node* if_after = label_new(literal_name_new());

    Node* check_cond_jmp = cond_goto_new(
        symbol_to_check->name, 
        if_true->name, 
        if_after->name,
        operation->token_type,
        upper_bound
    );

    Node* jmp_to_if_after = goto_new(if_after->name);

    Node* new_branch_block = node_new();
    new_branch_block->type = NODE_BLOCK;

    nodes_append_child(new_branch_block, check_cond_jmp);
    nodes_append_child(new_branch_block, if_true);
    nodes_extend_children(new_branch_block, block);
    nodes_extend_children(new_branch_block, jmp_to_if_after);
    nodes_append_child(new_branch_block, if_after);

    nodes_replace(curr_node, new_branch_block);

    log_tree(LOG_DEBUG, check_cond_jmp);
}

bool for_and_if_to_branch(Node* curr_node) {
    switch (curr_node->type) {
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

