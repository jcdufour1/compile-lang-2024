#include "../symbol_table.h"
#include "../parser_utils.h"
#include "../nodes.h"

static Node_label* label_new(Str_view label_name, Pos pos) {
    Node_label* label = node_unwrap_label(node_new(pos, NODE_LABEL));
    label->name = label_name;
    try(sym_tbl_add(node_wrap(label)));
    return label;
}

static Node_goto* goto_new(Str_view name_label_to_jmp_to, Pos pos) {
    Node_goto* lang_goto = node_unwrap_goto(node_new(pos, NODE_GOTO));
    lang_goto->name = name_label_to_jmp_to;
    return lang_goto;
}

static Node_cond_goto* conditional_goto_new(
    Node_operator* oper_rtn_sym,
    Str_view label_name_if_true,
    Str_view label_name_if_false
) {
    assert(!node_wrap(oper_rtn_sym)->prev);
    assert(!node_wrap(oper_rtn_sym)->next);
    assert(!node_wrap(oper_rtn_sym)->parent);

    Node_cond_goto* cond_goto = node_unwrap_cond_goto(node_new(node_wrap(oper_rtn_sym)->pos, NODE_COND_GOTO));
    cond_goto->node_src = oper_rtn_sym;
    cond_goto->if_true = symbol_new(label_name_if_true, node_wrap(oper_rtn_sym)->pos);
    cond_goto->if_false = symbol_new(label_name_if_false, node_wrap(oper_rtn_sym)->pos);
    return cond_goto;
}

static Node_assignment* get_for_loop_cond_var_assign(Str_view sym_name, Pos pos) {
    Node_literal* literal = literal_new(str_view_from_cstr("1"), TOKEN_NUM_LITERAL, pos);
    Node_operator* operation = operation_new(node_wrap(symbol_new(sym_name, pos)), node_wrap(literal), TOKEN_SINGLE_PLUS);
    return assignment_new(node_wrap(symbol_new(sym_name, pos)), node_wrap(operation));
}

static void change_break_to_goto(Node_block* block, const Node_label* label_to_goto) {
    Node* curr_node = nodes_get_local_rightmost(block->child);
    while (curr_node) {
        switch (curr_node->type) {
            case NODE_IF_STATEMENT:
                change_break_to_goto(node_unwrap_if(curr_node)->body, label_to_goto);
                break;
            case NODE_FUNCTION_CALL:
                break;
            case NODE_ASSIGNMENT:
                break;
            case NODE_FOR_LOOP:
                break;
            case NODE_VARIABLE_DEFINITION:
                break;
            case NODE_BREAK: {
                Node_goto* new_goto = goto_new(label_to_goto->name, node_wrap(label_to_goto)->pos);
                nodes_insert_before(curr_node, node_wrap(new_goto));
                nodes_remove(curr_node, false);
                curr_node = node_wrap(new_goto);
                break;
            }
            default:
                unreachable(NODE_FMT, node_print(curr_node));
        }

        curr_node = curr_node->prev;
    }
}

static void for_loop_to_branch(Node_for_loop* for_loop) {
    log_tree(LOG_TRACE, node_wrap(for_loop));
    Node_for_lower_bound* lhs = for_loop->lower_bound;
    Node_for_upper_bound* rhs = for_loop->upper_bound;
    Node_block* for_block = for_loop->body;

    Node* rhs_actual = nodes_single_child(node_wrap(rhs));
    nodes_remove(rhs_actual, true);

    Node_block* new_branch_block = node_unwrap_block(node_new(node_wrap(for_loop)->pos, NODE_BLOCK));
    assert(!node_wrap(new_branch_block)->parent);

    Node_symbol_untyped* symbol_lhs_assign;
    Node_variable_def* for_var_def;
    {
        for_var_def = for_loop->var_def;
        nodes_remove_siblings_and_parent(node_wrap(for_var_def));
        symbol_lhs_assign = symbol_new(for_var_def->name, node_wrap(for_var_def)->pos);
    }

    Node_assignment* assignment_to_inc_cond_var = get_for_loop_cond_var_assign(for_var_def->name, node_wrap(lhs)->pos);
    assert(!node_wrap(new_branch_block)->parent);
    nodes_remove(node_wrap(lhs), true);
    Node* lower_bound_child = nodes_single_child(node_wrap(lhs));
    nodes_remove(node_wrap(lower_bound_child), true);
    nodes_remove(node_wrap(rhs), true);
    nodes_remove(node_wrap(for_block), true);

    Node_operator* new_operation = operation_new(
        node_wrap(symbol_new(symbol_lhs_assign->name, node_wrap(symbol_lhs_assign)->pos)), rhs_actual, TOKEN_LESS_THAN
    );

    // initial assignment
    Node_assignment* new_var_assign = assignment_new(node_wrap(symbol_lhs_assign), node_wrap(lower_bound_child));

    Node_label* check_cond_label = label_new(literal_name_new(), node_wrap(for_loop)->pos);
    Node_goto* jmp_to_check_cond_label = goto_new(check_cond_label->name, node_wrap(for_loop)->pos);
    Node_label* after_check_label = label_new(literal_name_new(), node_wrap(for_loop)->pos);
    Node_label* after_for_loop_label = label_new(literal_name_new(), node_wrap(for_loop)->pos);
    Node_llvm_register_sym* oper_rtn_sym = node_unwrap_llvm_register_sym(node_new(node_wrap(new_operation)->pos, NODE_LLVM_REGISTER_SYM));
    oper_rtn_sym->node_src = node_wrap(new_operation);
    Node_cond_goto* check_cond_jmp = conditional_goto_new(
        new_operation,
        after_check_label->name, 
        after_for_loop_label->name
    );

    change_break_to_goto(for_block, after_for_loop_label);

    nodes_append_child(node_wrap(new_branch_block), node_wrap(for_var_def));
    nodes_append_child(node_wrap(new_branch_block), node_wrap(new_var_assign));
    nodes_append_child(node_wrap(new_branch_block), node_wrap(jmp_to_check_cond_label));
    nodes_append_child(node_wrap(new_branch_block), node_wrap(check_cond_label));
    nodes_append_child(node_wrap(new_branch_block), node_wrap(new_operation));
    nodes_append_child(node_wrap(new_branch_block), node_wrap(check_cond_jmp));
    nodes_append_child(node_wrap(new_branch_block), node_wrap(after_check_label));
    nodes_extend_children(node_wrap(new_branch_block), get_left_child(node_wrap(for_block)));
    nodes_append_child(node_wrap(new_branch_block), node_wrap(assignment_to_inc_cond_var));
    nodes_append_child(node_wrap(new_branch_block), node_wrap(goto_new(check_cond_label->name, node_wrap(for_loop)->pos)));
    nodes_append_child(node_wrap(new_branch_block), node_wrap(after_for_loop_label));

    nodes_insert_before(node_wrap(for_loop), node_wrap(new_branch_block));
}

static void if_statement_to_branch(Node_if* if_statement) {
    Node_if_condition* if_condition = if_statement->condition;
    nodes_remove(node_wrap(if_condition), true);
    assert(nodes_count_children(node_wrap(if_condition)) == 1);
    Node_block* block = if_statement->body;

    Node_operator* operation = node_unwrap_operator(nodes_single_child(node_wrap(if_condition)));
    nodes_remove(node_wrap(operation), true);

    Node_label* if_true = label_new(literal_name_new(), node_wrap(block)->pos);
    Node_label* if_after = label_new(literal_name_new(), node_wrap(operation)->pos);

    Node_cond_goto* check_cond_jmp = conditional_goto_new(operation, if_true->name, if_after->name);

    Node_goto* jmp_to_if_after = goto_new(if_after->name, node_wrap(block)->pos);

    Node_block* new_branch_block = node_unwrap_block(node_new(node_wrap(block)->pos, NODE_BLOCK));

    nodes_append_child(node_wrap(new_branch_block), node_wrap(operation));
    nodes_append_child(node_wrap(new_branch_block), node_wrap(check_cond_jmp));
    nodes_append_child(node_wrap(new_branch_block), node_wrap(if_true));
    nodes_extend_children(node_wrap(new_branch_block), node_wrap(block));
    nodes_extend_children(node_wrap(new_branch_block), node_wrap(jmp_to_if_after));
    nodes_append_child(node_wrap(new_branch_block), node_wrap(if_after));

    nodes_insert_before(node_wrap(if_statement), node_wrap(new_branch_block));
}

bool for_and_if_to_branch(Node* block_) {
    if (block_->type != NODE_BLOCK) {
        return false;
    }
    Node_block* block = node_unwrap_block(block_);
    Node* curr_node = nodes_get_local_rightmost(block->child);
    bool go_to_prev;
    (void) go_to_prev;
    while (curr_node) {
        bool go_to_prev = true;
        switch (curr_node->type) {
            case NODE_FOR_LOOP: {
                for_loop_to_branch(node_unwrap_for_loop(curr_node));
                Node* prev = curr_node->prev;
                nodes_remove(curr_node, true);
                curr_node = prev;
                go_to_prev = false;
                break;
            }
            case NODE_IF_STATEMENT: {
                if_statement_to_branch(node_unwrap_if(curr_node));
                Node* prev = curr_node->prev;
                nodes_remove(curr_node, true);
                curr_node = prev;
                go_to_prev = false;
                break;
            }
            case NODE_BREAK:
                unreachable("");
                break;
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

