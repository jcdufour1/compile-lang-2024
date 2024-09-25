#include "../symbol_table.h"
#include "../parser_utils.h"
#include "../nodes.h"

static Node* operation_new(Node* lhs, Node* rhs, TOKEN_TYPE operation_type) {
    // TODO: check if lhs or rhs were already appended to the tree
    Node* assignment = node_new(lhs->file_path, lhs->line_num);
    assignment->type = NODE_OPERATOR;
    assignment->token_type = operation_type;
    nodes_append_child(assignment, lhs);
    nodes_append_child(assignment, rhs);
    return assignment;
}

static Node* label_new(Str_view label_name, const char* file_path, uint32_t line_num) {
    Node* label = node_new(file_path, line_num);
    label->type = NODE_LABEL;
    label->name = label_name;
    try(sym_tbl_add(label));
    return label;
}

static Node* goto_new(Str_view name_label_to_jmp_to, const char* file_path, uint32_t line_num) {
    Node* lang_goto = node_new(file_path, line_num);
    lang_goto->type = NODE_GOTO;
    lang_goto->name = name_label_to_jmp_to;
    return lang_goto;
}

static Node* cond_goto_new(
    Node* lhs,
    Node* rhs,
    Str_view label_name_if_true,
    Str_view label_name_if_false,
    TOKEN_TYPE operator_type
) {
    assert(!lhs->prev);
    assert(!lhs->next);
    assert(!lhs->parent);
    assert(!rhs->prev);
    assert(!rhs->next);
    assert(!rhs->parent);

    Node* operator = node_new(lhs->file_path, lhs->line_num);
    operator->type = NODE_OPERATOR;
    operator->token_type = operator_type;
    nodes_append_child(operator, lhs);
    nodes_append_child(operator, rhs);

    Node* cond_goto = node_new(lhs->file_path, lhs->line_num);
    cond_goto->type = NODE_COND_GOTO;
    nodes_append_child(cond_goto, operator);
    nodes_append_child(cond_goto, symbol_new(label_name_if_true, lhs->file_path, lhs->line_num));
    nodes_append_child(cond_goto, symbol_new(label_name_if_false, lhs->file_path, lhs->line_num));
    return cond_goto;
}

static Node* get_for_loop_cond_var_assign(Str_view sym_name, const char* file_path, uint32_t line_num) {
    Node* literal = literal_new(str_view_from_cstr("1"), file_path, line_num);
    Node* operation = operation_new(symbol_new(sym_name, file_path, line_num), literal, TOKEN_SINGLE_PLUS);
    return assignment_new(symbol_new(sym_name, file_path, line_num), operation);
}

static void for_loop_to_branch(Node* for_loop) {
    log_tree(LOG_DEBUG, for_loop);
    Node* lhs = nodes_get_child_of_type(for_loop, NODE_FOR_LOWER_BOUND);
    Node* rhs = nodes_get_child_of_type(for_loop, NODE_FOR_UPPER_BOUND);
    Node* for_block = nodes_get_child_of_type(for_loop, NODE_BLOCK);

    Node* rhs_actual = nodes_single_child(rhs);
    nodes_remove_siblings_and_parent(rhs_actual);

    Node* new_branch_block = node_new(for_loop->file_path, for_loop->line_num);
    new_branch_block->type = NODE_BLOCK;
    assert(!new_branch_block->parent);

    Node* symbol_lhs_assign;
    Node* regular_var_def;
    {
        Node* new_var_def = nodes_get_child_of_type(for_loop, NODE_FOR_VARIABLE_DEF);
        regular_var_def = nodes_get_child_of_type(new_var_def, NODE_VARIABLE_DEFINITION);
        nodes_remove(regular_var_def, false);
        symbol_lhs_assign = symbol_new(regular_var_def->name, regular_var_def->file_path, regular_var_def->line_num);
        nodes_remove_siblings(new_var_def);
    }

    Node* assignment_to_inc_cond_var = get_for_loop_cond_var_assign(regular_var_def->name, lhs->file_path, lhs->line_num);
    assert(!new_branch_block->parent);
    nodes_remove_siblings_and_parent(lhs);
    Node* lower_bound_child = nodes_single_child(lhs);
    nodes_remove_siblings_and_parent(lower_bound_child);
    nodes_remove_siblings_and_parent(rhs);
    nodes_remove_siblings_and_parent(for_block);

    // initial assignment
    Node* new_var_assign = assignment_new(symbol_lhs_assign, lower_bound_child);

    Node* check_cond_label = label_new(literal_name_new(), for_loop->file_path, for_loop->line_num);
    Node* jmp_to_check_cond_label = goto_new(check_cond_label->name, for_loop->file_path, for_loop->line_num);
    Node* after_check_label = label_new(literal_name_new(), for_loop->file_path, for_loop->line_num);
    Node* after_for_loop_label = label_new(literal_name_new(), for_loop->file_path, for_loop->line_num);
    Node* check_cond_jmp = cond_goto_new(
        symbol_new(regular_var_def->name, lhs->file_path, lhs->line_num),
        rhs_actual,
        after_check_label->name, 
        after_for_loop_label->name,
        TOKEN_LESS_THAN
    );

    nodes_append_child(new_branch_block, regular_var_def);
    nodes_append_child(new_branch_block, new_var_assign);
    nodes_append_child(new_branch_block, jmp_to_check_cond_label);
    nodes_append_child(new_branch_block, check_cond_label);
    nodes_append_child(new_branch_block, check_cond_jmp);
    nodes_append_child(new_branch_block, after_check_label);
    nodes_extend_children(new_branch_block, for_block->left_child);
    nodes_append_child(new_branch_block, assignment_to_inc_cond_var);
    nodes_append_child(new_branch_block, goto_new(check_cond_label->name, for_loop->file_path, for_loop->line_num));
    nodes_append_child(new_branch_block, after_for_loop_label);

    nodes_insert_before(for_loop, new_branch_block);
}

static void if_statement_to_branch(Node* if_statement) {
    Node* condition = nodes_get_child_of_type(if_statement, NODE_IF_CONDITION);
    Node* block = nodes_get_child_of_type(if_statement, NODE_BLOCK);

    Node* operation = nodes_get_child_of_type(condition, NODE_OPERATOR);
    nodes_remove(operation, true);
    Node* lhs = nodes_get_child(operation, 0);
    Node* rhs = nodes_get_child(operation, 1);
    nodes_remove(lhs, true);
    nodes_remove(rhs, true);

    Node* if_true = label_new(literal_name_new(), block->file_path, block->line_num);
    Node* if_after = label_new(literal_name_new(), operation->file_path, operation->line_num);

    Node* check_cond_jmp = cond_goto_new(
        lhs, 
        rhs,
        if_true->name, 
        if_after->name,
        operation->token_type
    );

    Node* jmp_to_if_after = goto_new(if_after->name, block->file_path, block->line_num);

    Node* new_branch_block = node_new(block->file_path, block->line_num);
    new_branch_block->type = NODE_BLOCK;

    nodes_append_child(new_branch_block, check_cond_jmp);
    nodes_append_child(new_branch_block, if_true);
    nodes_extend_children(new_branch_block, block);
    nodes_extend_children(new_branch_block, jmp_to_if_after);
    nodes_append_child(new_branch_block, if_after);

    nodes_insert_before(if_statement, new_branch_block);
}

bool for_and_if_to_branch(Node* block) {
    if (block->type != NODE_BLOCK) {
        return false;
    }
    Node* curr_node = nodes_get_local_rightmost(block->left_child);
    bool go_to_prev;
    while (curr_node) {
        bool go_to_prev = true;
        switch (curr_node->type) {
            case NODE_FOR_LOOP: {
                for_loop_to_branch(curr_node);
                Node* prev = curr_node->prev;
                nodes_remove(curr_node, true);
                curr_node = prev;
                go_to_prev = false;
                break;
            }
            case NODE_IF_STATEMENT: {
                if_statement_to_branch(curr_node);
                Node* prev = curr_node->prev;
                nodes_remove(curr_node, true);
                curr_node = prev;
                go_to_prev = false;
                break;
            }
            default:
                break;
        }

        // TODO: is `unused` here a clang bug? should I report it?
        if (go_to_prev) {
            curr_node = curr_node->prev;
        }
    }

    return true;
}

