#include "../symbol_table.h"
#include "../parser_utils.h"
#include "../nodes.h"
#include "passes.h"

static Node_label* label_new(Env* env, Str_view label_name, Pos pos) {
    Node_label* label = node_unwrap_label(node_new(pos, NODE_LABEL));
    label->name = label_name;
    try(symbol_add(env, node_wrap_label(label)));
    return label;
}

static Node_goto* goto_new(Str_view name_label_to_jmp_to, Pos pos) {
    Node_goto* lang_goto = node_unwrap_goto(node_new(pos, NODE_GOTO));
    lang_goto->name = name_label_to_jmp_to;
    return lang_goto;
}

static Node_cond_goto* conditional_goto_new(
    Node_e_operator* operator,
    Str_view label_name_if_true,
    Str_view label_name_if_false
) {
    Node_cond_goto* cond_goto = node_unwrap_cond_goto(node_new(node_wrap_expr(node_wrap_e_operator(operator))->pos, NODE_COND_GOTO));
    cond_goto->node_src = operator;
    cond_goto->if_true = symbol_new(label_name_if_true, node_wrap_expr(node_wrap_e_operator(operator))->pos);
    cond_goto->if_false = symbol_new(label_name_if_false, node_wrap_expr(node_wrap_e_operator(operator))->pos);
    return cond_goto;
}

static Node_assignment* for_loop_cond_var_assign_new(Env* env, Str_view sym_name, Pos pos) {
    Node_e_literal* literal = literal_new(str_view_from_cstr("1"), TOKEN_INT_LITERAL, pos);
    Node_expr* operator = binary_new(env, node_wrap_e_symbol_untyped(symbol_new(sym_name, pos)), node_wrap_e_literal(literal), TOKEN_SINGLE_PLUS);
    return assignment_new(env, node_wrap_expr(node_wrap_e_symbol_untyped(symbol_new(sym_name, pos))), operator);
}

static void change_break_to_goto(Node_block* block, const Node_label* label_to_goto) {
    for (size_t idx_ = block->children.info.count; idx_ > 0; idx_--) {
        size_t idx = idx_ - 1;

        Node** curr_node = vec_at_ref(&block->children, idx);

        log_tree(LOG_DEBUG, *curr_node);

        if ((*curr_node)->type == NODE_EXPR) {
            Node_expr* expr = node_unwrap_expr(*curr_node);
            switch (expr->type) {
                case NODE_E_FUNCTION_CALL:
                    break;
                default:
                    todo();
            }
        } else {
            switch ((*curr_node)->type) {
                case NODE_IF:
                    change_break_to_goto(node_unwrap_if(*curr_node)->body, label_to_goto);
                    break;
                case NODE_ASSIGNMENT:
                    break;
                case NODE_FOR_RANGE:
                    break;
                case NODE_FOR_WITH_COND:
                    break;
                case NODE_VARIABLE_DEF:
                    break;
                case NODE_BREAK:
                    *curr_node = node_wrap_goto(goto_new(label_to_goto->name, node_wrap_label_const(label_to_goto)->pos));
                    assert(*curr_node);
                    break;
                default:
                    log_tree(LOG_DEBUG, *curr_node);
                    unreachable("");
            }
        }
    }
}

static Node_block* for_with_cond_to_branch(Env* env, Node_for_with_cond* for_loop) {
    Node_block* for_block = for_loop->body;
    Node_block* new_branch_block = node_unwrap_block(node_new(node_wrap_for_with_cond(for_loop)->pos, NODE_BLOCK));
    Node_e_operator* operator = node_unwrap_e_operator(for_loop->condition->child);

    Node_label* check_cond_label = label_new(env, literal_name_new(), node_wrap_for_with_cond(for_loop)->pos);
    Node_goto* jmp_to_check_cond_label = goto_new(check_cond_label->name, node_wrap_for_with_cond(for_loop)->pos);
    Node_label* after_check_label = label_new(env, literal_name_new(), node_wrap_for_with_cond(for_loop)->pos);
    Node_label* after_for_loop_label = label_new(env, literal_name_new(), node_wrap_for_with_cond(for_loop)->pos);
    Llvm_register_sym oper_rtn_sym = llvm_register_sym_new_from_expr(node_wrap_e_operator(operator));
    oper_rtn_sym.node = node_wrap_expr(node_wrap_e_operator(operator));
    Node_cond_goto* check_cond_jmp = conditional_goto_new(
        operator,
        after_check_label->name, 
        after_for_loop_label->name
    );

    change_break_to_goto(for_block, after_for_loop_label);

    vec_append(&a_main, &new_branch_block->children, node_wrap_goto(jmp_to_check_cond_label));
    vec_append(&a_main, &new_branch_block->children, node_wrap_label(check_cond_label));
    vec_append(&a_main, &new_branch_block->children, node_wrap_expr(node_wrap_e_operator(operator)));
    vec_append(&a_main, &new_branch_block->children, node_wrap_cond_goto(check_cond_jmp));
    vec_append(&a_main, &new_branch_block->children, node_wrap_label(after_check_label));
    vec_extend(&a_main, &new_branch_block->children, &for_block->children);
    vec_append(&a_main, &new_branch_block->children, node_wrap_goto(goto_new(check_cond_label->name, node_wrap_for_with_cond(for_loop)->pos)));
    vec_append(&a_main, &new_branch_block->children, node_wrap_label(after_for_loop_label));

    return new_branch_block;
}

static Node_block* for_range_to_branch(Env* env, Node_for_range* for_loop) {
    log_tree(LOG_TRACE, node_wrap_for_range(for_loop));

    Node_block* for_block = for_loop->body;
    vec_append(&a_main, &env->ancesters, node_wrap_block(for_block));
    symbol_log(LOG_DEBUG, env);
    Node_expr* lhs_actual = for_loop->lower_bound->child;
    Node_expr* rhs_actual = for_loop->upper_bound->child;

    Node_block* new_branch_block = node_unwrap_block(node_new(node_wrap_for_range(for_loop)->pos, NODE_BLOCK));

    Node_e_symbol_untyped* symbol_lhs_assign;
    Node_variable_def* for_var_def;
    {
        for_var_def = for_loop->var_def;
        symbol_lhs_assign = symbol_new(for_var_def->name, node_wrap_variable_def(for_var_def)->pos);
    }

    Node_assignment* assignment_to_inc_cond_var = for_loop_cond_var_assign_new(env, for_var_def->name, node_wrap_expr(lhs_actual)->pos);

    Node_expr* operator = binary_new(
        env, node_wrap_e_symbol_untyped(symbol_new(symbol_lhs_assign->name, node_wrap_expr(node_wrap_e_symbol_untyped(symbol_lhs_assign))->pos)), rhs_actual, TOKEN_LESS_THAN
    );

    // initial assignment
    Node_assignment* new_var_assign = assignment_new(env, node_wrap_expr(node_wrap_e_symbol_untyped(symbol_lhs_assign)), lhs_actual);

    Node_label* check_cond_label = label_new(env, literal_name_new(), node_wrap_for_range(for_loop)->pos);
    Node_goto* jmp_to_check_cond_label = goto_new(check_cond_label->name, node_wrap_for_range(for_loop)->pos);
    Node_label* after_check_label = label_new(env, literal_name_new(), node_wrap_for_range(for_loop)->pos);
    Node_label* after_for_loop_label = label_new(env, literal_name_new(), node_wrap_for_range(for_loop)->pos);
    Llvm_register_sym oper_rtn_sym = llvm_register_sym_new_from_expr(operator);
    oper_rtn_sym.node = node_wrap_expr(operator);
    Node_cond_goto* check_cond_jmp = conditional_goto_new(
        node_unwrap_e_operator(operator),
        after_check_label->name, 
        after_for_loop_label->name
    );

    change_break_to_goto(for_block, after_for_loop_label);

    vec_append(&a_main, &new_branch_block->children, node_wrap_variable_def(for_var_def));
    vec_append(&a_main, &new_branch_block->children, node_wrap_assignment(new_var_assign));
    vec_append(&a_main, &new_branch_block->children, node_wrap_goto(jmp_to_check_cond_label));
    vec_append(&a_main, &new_branch_block->children, node_wrap_label(check_cond_label));
    vec_append(&a_main, &new_branch_block->children, node_wrap_expr(operator));
    vec_append(&a_main, &new_branch_block->children, node_wrap_cond_goto(check_cond_jmp));
    vec_append(&a_main, &new_branch_block->children, node_wrap_label(after_check_label));
    vec_extend(&a_main, &new_branch_block->children, &for_block->children);
    vec_append(&a_main, &new_branch_block->children, node_wrap_assignment(assignment_to_inc_cond_var));
    vec_append(&a_main, &new_branch_block->children, node_wrap_goto(goto_new(check_cond_label->name, node_wrap_for_range(for_loop)->pos)));
    vec_append(&a_main, &new_branch_block->children, node_wrap_label(after_for_loop_label));

    new_branch_block->symbol_table = for_block->symbol_table;
    vec_rem_last(&env->ancesters);
    return new_branch_block;
}

static Node_block* if_statement_to_branch(Env* env, Node_if* if_statement, Node_label* next_if, Node_label* after_chain) {
    Node_condition* if_cond = if_statement->condition;
    Node_block* old_block = if_statement->body;

    Node_block* new_block = node_unwrap_block(node_new(node_wrap_block(old_block)->pos, NODE_BLOCK));

    Node_e_operator* operator;
    switch (if_cond->child->type) {
        case NODE_E_OPERATOR:
            operator = node_unwrap_e_operator(if_cond->child);

            Node_label* if_body = label_new(env, literal_name_new(), node_wrap_block(old_block)->pos);

            Node_cond_goto* check_cond_jmp = conditional_goto_new(operator, if_body->name, next_if->name);

            vec_append(&a_main, &new_block->children, node_wrap_expr(node_wrap_e_operator(operator)));
            vec_append(&a_main, &new_block->children, node_wrap_cond_goto(check_cond_jmp));
            vec_append(&a_main, &new_block->children, node_wrap_label(if_body));
            vec_extend(&a_main, &new_block->children, &old_block->children);

            Node_goto* jmp_to_after_chain = goto_new(after_chain->name, node_wrap_block(old_block)->pos);
            vec_append(&a_main, &new_block->children, node_wrap_goto(jmp_to_after_chain));

            break;
        case NODE_E_LITERAL: {
            unreachable("");
            const Node_e_literal* literal = node_unwrap_e_literal_const(if_cond->child);
            int64_t value = node_unwrap_lit_number_const(literal)->data;
            if (value == 0) {
                return new_block;
            } else {
                vec_extend(&a_main, &new_block->children, &old_block->children);
            }
            break;
        }
        default:
            unreachable("");
    }

    return new_block;
}

static Node_block* if_else_chain_to_branch(Env* env, Node_if_else_chain* if_else) {
    Node_label* if_after = label_new(env, literal_name_new(), node_wrap_if_else_chain(if_else)->pos);
    Node_block* new_block = node_unwrap_block(node_new(node_wrap_if_else_chain(if_else)->pos, NODE_BLOCK));

    Node_label* next_if = NULL;
    for (size_t idx = 0; idx < if_else->nodes.info.count; idx++) {
        if (idx + 1 == if_else->nodes.info.count) {
            next_if = if_after;
        } else {
            next_if = label_new(env, literal_name_new(), node_wrap_if(vec_at(&if_else->nodes, idx))->pos);
        }

        log(LOG_DEBUG, "yes\n");
        Node_block* if_block = if_statement_to_branch(env, vec_at(&if_else->nodes, idx), next_if, if_after);
        assert(if_block->children.info.count > 0);
        vec_append(&a_main, &new_block->children, node_wrap_block(if_block));

        if (idx + 1 < if_else->nodes.info.count) {
            vec_append(&a_main, &new_block->children, node_wrap_label(next_if));
        }

        log_tree(LOG_DEBUG, node_wrap_block(if_block));
        log_tree(LOG_DEBUG, node_wrap_block(new_block));
    }

    vec_append(&a_main, &new_block->children, node_wrap_label(if_after));
    log_tree(LOG_DEBUG, node_wrap_block(new_block));

    return new_block;
}

void for_and_if_to_branch(Env* env) {
    Node* block_ = vec_top(&env->ancesters);
    if (block_->type != NODE_BLOCK) {
        return;
    }
    Node_block* block = node_unwrap_block(block_);

    if (block->children.info.count < 1) {
        return;
    }
    for (size_t idx = block->children.info.count - 1;; idx--) {
        Node** curr_node = vec_at_ref(&block->children, idx);

        switch ((*curr_node)->type) {
            case NODE_FOR_RANGE:
                *curr_node = node_wrap_block(for_range_to_branch(env, node_unwrap_for_range(*curr_node)));
                break;
            case NODE_FOR_WITH_COND:
                *curr_node = node_wrap_block(for_with_cond_to_branch(env, node_unwrap_for_with_cond(*curr_node)));
                assert(*curr_node);
                break;
            case NODE_IF:
                unreachable("");
            case NODE_IF_ELSE_CHAIN:
                *curr_node = node_wrap_block(if_else_chain_to_branch(env, node_unwrap_if_else_chain(*curr_node)));
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

    return;
}

