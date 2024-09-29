#include "../symbol_table.h"
#include "../parser_utils.h"
#include "../nodes.h"

static Node* operation_new(Node* lhs, Node* rhs, TOKEN_TYPE operation_type) {
    // TODO: check if lhs or rhs were already appended to the tree
    Node* assignment = node_new(lhs->pos);
    assignment->type = NODE_OPERATOR;
    assignment->token_type = operation_type;
    nodes_append_child(assignment, lhs);
    nodes_append_child(assignment, rhs);

    set_assignment_operand_types(lhs, rhs);
    return assignment;
}

static Node* label_new(Str_view label_name, Pos pos) {
    Node* label = node_new(pos);
    label->type = NODE_LABEL;
    label->name = label_name;
    try(sym_tbl_add(label));
    return label;
}

static Node* goto_new(Str_view name_label_to_jmp_to, Pos pos) {
    Node* lang_goto = node_new(pos);
    lang_goto->type = NODE_GOTO;
    lang_goto->name = name_label_to_jmp_to;
    return lang_goto;
}

static Node* conditional_goto_new(
    Node* oper_rtn_sym,
    Str_view label_name_if_true,
    Str_view label_name_if_false
) {
    assert(oper_rtn_sym->type == NODE_OPERATOR_RETURN_VALUE_SYM);
    assert(!oper_rtn_sym->prev);
    assert(!oper_rtn_sym->next);
    assert(!oper_rtn_sym->parent);

    Node* cond_goto = node_new(oper_rtn_sym->pos);
    cond_goto->type = NODE_COND_GOTO;
    nodes_append_child(cond_goto, oper_rtn_sym);
    nodes_append_child(cond_goto, symbol_new(label_name_if_true, oper_rtn_sym->pos));
    nodes_append_child(cond_goto, symbol_new(label_name_if_false, oper_rtn_sym->pos));
    return cond_goto;
}

static Node* get_for_loop_cond_var_assign(Str_view sym_name, Pos pos) {
    Node* literal = literal_new(str_view_from_cstr("1"), TOKEN_NUM_LITERAL, pos);
    Node* operation = operation_new(symbol_new(sym_name, pos), literal, TOKEN_SINGLE_PLUS);
    return assignment_new(symbol_new(sym_name, pos), operation);
}

static void for_loop_to_branch(Node* for_loop) {
    log_tree(LOG_TRACE, for_loop);
    Node* lhs = nodes_get_child_of_type(for_loop, NODE_FOR_LOWER_BOUND);
    Node* rhs = nodes_get_child_of_type(for_loop, NODE_FOR_UPPER_BOUND);
    Node* for_block = nodes_get_child_of_type(for_loop, NODE_BLOCK);

    Node* rhs_actual = nodes_single_child(rhs);
    nodes_remove(rhs_actual, true);

    Node* new_branch_block = node_new(for_loop->pos);
    new_branch_block->type = NODE_BLOCK;
    assert(!new_branch_block->parent);

    Node* symbol_lhs_assign;
    Node* regular_var_def;
    {
        Node* new_var_def = nodes_get_child_of_type(for_loop, NODE_FOR_VARIABLE_DEF);
        regular_var_def = nodes_get_child_of_type(new_var_def, NODE_VARIABLE_DEFINITION);
        nodes_remove(regular_var_def, false);
        symbol_lhs_assign = symbol_new(regular_var_def->name, regular_var_def->pos);
        nodes_remove_siblings(new_var_def);
    }

    Node* assignment_to_inc_cond_var = get_for_loop_cond_var_assign(regular_var_def->name, lhs->pos);
    assert(!new_branch_block->parent);
    nodes_remove(lhs, true);
    Node* lower_bound_child = nodes_single_child(lhs);
    nodes_remove(lower_bound_child, true);
    nodes_remove(rhs, true);
    nodes_remove(for_block, true);

    Node* new_operation = operation_new(
        symbol_new(symbol_lhs_assign->name, symbol_lhs_assign->pos),
        rhs_actual,
        TOKEN_LESS_THAN
    );

    // initial assignment
    Node* new_var_assign = assignment_new(symbol_lhs_assign, lower_bound_child);

    Node* check_cond_label = label_new(literal_name_new(), for_loop->pos);
    Node* jmp_to_check_cond_label = goto_new(check_cond_label->name, for_loop->pos);
    Node* after_check_label = label_new(literal_name_new(), for_loop->pos);
    Node* after_for_loop_label = label_new(literal_name_new(), for_loop->pos);
    Node* oper_rtn_sym = node_new(new_operation->pos);
    oper_rtn_sym->type = NODE_OPERATOR_RETURN_VALUE_SYM;
    oper_rtn_sym->node_src = new_operation;
    Node* check_cond_jmp = conditional_goto_new(
        oper_rtn_sym,
        after_check_label->name, 
        after_for_loop_label->name
    );

    nodes_append_child(new_branch_block, regular_var_def);
    nodes_append_child(new_branch_block, new_var_assign);
    nodes_append_child(new_branch_block, jmp_to_check_cond_label);
    nodes_append_child(new_branch_block, check_cond_label);
    nodes_append_child(new_branch_block, new_operation);
    nodes_append_child(new_branch_block, check_cond_jmp);
    nodes_append_child(new_branch_block, after_check_label);
    nodes_extend_children(new_branch_block, for_block->left_child);
    nodes_append_child(new_branch_block, assignment_to_inc_cond_var);
    nodes_append_child(new_branch_block, goto_new(check_cond_label->name, for_loop->pos));
    nodes_append_child(new_branch_block, after_for_loop_label);

    nodes_insert_before(for_loop, new_branch_block);
}

static void if_statement_to_branch(Node* if_statement) {
    Node* if_condition = nodes_get_child_of_type(if_statement, NODE_IF_CONDITION);
    nodes_remove(if_condition, true);
    assert(nodes_count_children(if_condition) == 1);
    Node* block = nodes_get_child_of_type(if_statement, NODE_BLOCK);

    Node* operation = nodes_single_child(if_condition);
    nodes_remove(operation, true);
    Node* oper_rtn_sym = node_new(operation->pos);
    oper_rtn_sym->type = NODE_OPERATOR_RETURN_VALUE_SYM;
    oper_rtn_sym->node_src = operation;

    Node* if_true = label_new(literal_name_new(), block->pos);
    Node* if_after = label_new(literal_name_new(), operation->pos);

    Node* check_cond_jmp = conditional_goto_new(oper_rtn_sym, if_true->name, if_after->name);

    Node* jmp_to_if_after = goto_new(if_after->name, block->pos);

    Node* new_branch_block = node_new(block->pos);
    new_branch_block->type = NODE_BLOCK;

    nodes_append_child(new_branch_block, operation);
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
    (void) go_to_prev;
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

