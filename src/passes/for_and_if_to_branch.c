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
    Node_operator* operator, // TODO: make better way to pass generic operation to a function
    Str_view label_name_if_true,
    Str_view label_name_if_false
) {
    Node_cond_goto* cond_goto = node_unwrap_cond_goto(node_new(node_wrap(operator)->pos, NODE_COND_GOTO));
    cond_goto->node_src = operator;
    cond_goto->if_true = symbol_new(label_name_if_true, node_wrap(operator)->pos);
    cond_goto->if_false = symbol_new(label_name_if_false, node_wrap(operator)->pos);
    return cond_goto;
}

static Node_assignment* for_loop_cond_var_assign_new(Str_view sym_name, Pos pos) {
    Node_literal* literal = literal_new(str_view_from_cstr("1"), TOKEN_NUM_LITERAL, pos);
    Node_binary* operation = binary_new(node_wrap(symbol_new(sym_name, pos)), node_wrap(literal), TOKEN_SINGLE_PLUS);
    return assignment_new(node_wrap(symbol_new(sym_name, pos)), node_wrap(operation));
}

static void change_break_to_goto(Node_block* block, const Node_label* label_to_goto) {
    for (size_t idx_ = block->children.info.count; idx_ > 0; idx_--) {
        size_t idx = idx_ - 1;

        Node** curr_node = vec_at_ref(&block->children, idx);

        switch ((*curr_node)->type) {
            case NODE_IF_STATEMENT:
                change_break_to_goto(node_unwrap_if(*curr_node)->body, label_to_goto);
                break;
            case NODE_FUNCTION_CALL:
                break;
            case NODE_ASSIGNMENT:
                break;
            case NODE_FOR_RANGE:
                break;
            case NODE_FOR_WITH_CONDITION:
                break;
            case NODE_VARIABLE_DEFINITION:
                break;
            case NODE_BREAK:
                *curr_node = node_wrap(goto_new(label_to_goto->name, node_wrap(label_to_goto)->pos));
                break;
            default:
                unreachable(NODE_FMT, node_print(curr_node));
        }
    }
}

static Node_block* for_with_condition_to_branch(Node_for_with_condition* for_loop) {
    Node_block* for_block = for_loop->body;
    Node_block* new_branch_block = node_unwrap_block(node_new(node_wrap(for_loop)->pos, NODE_BLOCK));
    Node_operator* operation = node_unwrap_operation(for_loop->condition->child);

    Node_label* check_cond_label = label_new(literal_name_new(), node_wrap(for_loop)->pos);
    Node_goto* jmp_to_check_cond_label = goto_new(check_cond_label->name, node_wrap(for_loop)->pos);
    Node_label* after_check_label = label_new(literal_name_new(), node_wrap(for_loop)->pos);
    Node_label* after_for_loop_label = label_new(literal_name_new(), node_wrap(for_loop)->pos);
    Node_llvm_register_sym* oper_rtn_sym = node_unwrap_llvm_register_sym(node_new(node_wrap(operation)->pos, NODE_LLVM_REGISTER_SYM));
    oper_rtn_sym->node_src = node_wrap(operation);
    Node_cond_goto* check_cond_jmp = conditional_goto_new(
        operation,
        after_check_label->name, 
        after_for_loop_label->name
    );

    change_break_to_goto(for_block, after_for_loop_label);

    vec_append(&a_main, &new_branch_block->children, node_wrap(jmp_to_check_cond_label));
    vec_append(&a_main, &new_branch_block->children, node_wrap(check_cond_label));
    vec_append(&a_main, &new_branch_block->children, node_wrap(operation));
    vec_append(&a_main, &new_branch_block->children, node_wrap(check_cond_jmp));
    vec_append(&a_main, &new_branch_block->children, node_wrap(after_check_label));
    vec_extend(&a_main, &new_branch_block->children, &for_block->children);
    vec_append(&a_main, &new_branch_block->children, node_wrap(goto_new(check_cond_label->name, node_wrap(for_loop)->pos)));
    vec_append(&a_main, &new_branch_block->children, node_wrap(after_for_loop_label));

    return new_branch_block;
}

static Node_block* for_range_to_branch(Node_for_range* for_loop) {
    log_tree(LOG_TRACE, node_wrap(for_loop));

    Node_block* for_block = for_loop->body;
    Node* lhs_actual = for_loop->lower_bound->child;
    Node* rhs_actual = for_loop->upper_bound->child;

    Node_block* new_branch_block = node_unwrap_block(node_new(node_wrap(for_loop)->pos, NODE_BLOCK));

    Node_symbol_untyped* symbol_lhs_assign;
    Node_variable_def* for_var_def;
    {
        for_var_def = for_loop->var_def;
        symbol_lhs_assign = symbol_new(for_var_def->name, node_wrap(for_var_def)->pos);
    }

    Node_assignment* assignment_to_inc_cond_var = for_loop_cond_var_assign_new(for_var_def->name, node_wrap(lhs_actual)->pos);

    Node_binary* new_operation = binary_new(
        node_wrap(symbol_new(symbol_lhs_assign->name, node_wrap(symbol_lhs_assign)->pos)), rhs_actual, TOKEN_LESS_THAN
    );

    // initial assignment
    Node_assignment* new_var_assign = assignment_new(node_wrap(symbol_lhs_assign), node_wrap(lhs_actual));

    Node_label* check_cond_label = label_new(literal_name_new(), node_wrap(for_loop)->pos);
    Node_goto* jmp_to_check_cond_label = goto_new(check_cond_label->name, node_wrap(for_loop)->pos);
    Node_label* after_check_label = label_new(literal_name_new(), node_wrap(for_loop)->pos);
    Node_label* after_for_loop_label = label_new(literal_name_new(), node_wrap(for_loop)->pos);
    Node_llvm_register_sym* oper_rtn_sym = node_unwrap_llvm_register_sym(node_new(node_wrap(new_operation)->pos, NODE_LLVM_REGISTER_SYM));
    oper_rtn_sym->node_src = node_wrap(new_operation);
    Node_cond_goto* check_cond_jmp = conditional_goto_new(
        node_wrap_operator(new_operation),
        after_check_label->name, 
        after_for_loop_label->name
    );

    change_break_to_goto(for_block, after_for_loop_label);

    vec_append(&a_main, &new_branch_block->children, node_wrap(for_var_def));
    vec_append(&a_main, &new_branch_block->children, node_wrap(new_var_assign));
    vec_append(&a_main, &new_branch_block->children, node_wrap(jmp_to_check_cond_label));
    vec_append(&a_main, &new_branch_block->children, node_wrap(check_cond_label));
    vec_append(&a_main, &new_branch_block->children, node_wrap(new_operation));
    vec_append(&a_main, &new_branch_block->children, node_wrap(check_cond_jmp));
    vec_append(&a_main, &new_branch_block->children, node_wrap(after_check_label));
    vec_extend(&a_main, &new_branch_block->children, &for_block->children);
    vec_append(&a_main, &new_branch_block->children, node_wrap(assignment_to_inc_cond_var));
    vec_append(&a_main, &new_branch_block->children, node_wrap(goto_new(check_cond_label->name, node_wrap(for_loop)->pos)));
    vec_append(&a_main, &new_branch_block->children, node_wrap(after_for_loop_label));

    return new_branch_block;
}

static Node_block* if_statement_to_branch(Node_if* if_statement) {
    Node_if_condition* if_condition = if_statement->condition;
    Node_block* block = if_statement->body;

    Node_operator* operation = node_unwrap_operation(if_condition->child);

    Node_label* if_true = label_new(literal_name_new(), node_wrap(block)->pos);
    Node_label* if_after = label_new(literal_name_new(), node_wrap(operation)->pos);

    Node_cond_goto* check_cond_jmp = conditional_goto_new(operation, if_true->name, if_after->name);

    Node_goto* jmp_to_if_after = goto_new(if_after->name, node_wrap(block)->pos);

    Node_block* new_branch_block = node_unwrap_block(node_new(node_wrap(block)->pos, NODE_BLOCK));

    vec_append(&a_main, &new_branch_block->children, node_wrap(operation));
    vec_append(&a_main, &new_branch_block->children, node_wrap(check_cond_jmp));
    vec_append(&a_main, &new_branch_block->children, node_wrap(if_true));
    vec_extend(&a_main, &new_branch_block->children, &block->children);
    vec_append(&a_main, &new_branch_block->children, node_wrap(jmp_to_if_after));
    vec_append(&a_main, &new_branch_block->children, node_wrap(if_after));

    return new_branch_block;
}

bool for_and_if_to_branch(Node* block_) {
    if (block_->type != NODE_BLOCK) {
        return false;
    }
    Node_block* block = node_unwrap_block(block_);

    for (size_t idx = block->children.info.count - 1;; idx--) {
        Node** curr_node = vec_at_ref(&block->children, idx);

        switch ((*curr_node)->type) {
            case NODE_FOR_RANGE:
                *curr_node = node_wrap(for_range_to_branch(node_unwrap_for_range(*curr_node)));
                assert(*curr_node);
                break;
            case NODE_FOR_WITH_CONDITION:
                *curr_node = node_wrap(for_with_condition_to_branch(node_unwrap_for_with_condition(*curr_node)));
                assert(*curr_node);
                break;
            case NODE_IF_STATEMENT:
                *curr_node = node_wrap(if_statement_to_branch(node_unwrap_if(*curr_node)));
                assert(*curr_node);
                break;
            case NODE_BREAK:
                unreachable("");
                break;
            default:
                break;
        }

        if (idx < 1) {
            break;
        }
    }

    return true;
}

