#include "passes.h"
#include "../parser_utils.h"
#include "../nodes.h"
#include "../symbol_table.h"

// returns operand or operand symbol
static Node* flatten_operation_operand(
    Node_ptr_vec* block_children,
    size_t* idx_to_insert_before,
    Node* operand
) {
    if (operand->type == NODE_OPERATOR) {
        Node* operator_sym = node_new(operand->pos, NODE_LLVM_REGISTER_SYM);
        node_ptr_vec_insert(block_children, *idx_to_insert_before, operand);
        (*idx_to_insert_before)++;
        node_unwrap_llvm_register_sym(operator_sym)->node_src = operand;
        assert(node_unwrap_llvm_register_sym(operator_sym)->node_src);
        return operator_sym;
    } else if (operand->type == NODE_FUNCTION_CALL) {
        Node_llvm_register_sym* fun_sym = node_unwrap_llvm_register_sym(node_new(operand->pos, NODE_LLVM_REGISTER_SYM));
        node_ptr_vec_insert(block_children, *idx_to_insert_before, operand);
        (*idx_to_insert_before)++;
        fun_sym->node_src = operand;
        assert(fun_sym->node_src);
        return node_wrap(fun_sym);
    } else {
        assert(operand);
        return operand;
    }
}

static void flatten_operation_if_nessessary(
    Node_ptr_vec* block_children,
    size_t* idx_to_insert_before,
    Node_operator* old_operation
) {
    old_operation->lhs = flatten_operation_operand(block_children, idx_to_insert_before, old_operation->lhs);
    old_operation->rhs = flatten_operation_operand(block_children, idx_to_insert_before, old_operation->rhs);
}

// operator symbol is returned
static Node_llvm_register_sym* move_operator_back(
    Node_ptr_vec* block_children,
    size_t* idx_to_insert_before,
    Node_operator* operation
) {
    Node_llvm_register_sym* operator_sym = node_unwrap_llvm_register_sym(node_new(node_wrap(operation)->pos, NODE_LLVM_REGISTER_SYM));
    operator_sym->node_src = node_wrap(operation);
    try(try_set_operator_lang_type(operation));
    operator_sym->lang_type = operation->lang_type;
    assert(operator_sym->lang_type.str.count > 0);
    assert(operation);
    node_ptr_vec_insert(block_children, *idx_to_insert_before, node_wrap(operation));
    (*idx_to_insert_before)++;
    assert(operator_sym);
    return operator_sym;
}

static void swap_nodes(Node** a, Node** b) {
    Node* temp = *a;
    *a = *b;
    *b = temp;

    assert(*a);
    assert(*b);
}

bool flatten_operations(Node* block_, int recursion_depth) {
    (void) recursion_depth;
    if (block_->type != NODE_BLOCK) {
        return false;
    }
    Node_block* block = node_unwrap_block(block_);

    for (size_t idx = 0; idx < block->children.info.count; idx++) {
        assert(node_ptr_vec_at(&block->children, idx));
    }
    node_ptr_assert_no_null(&block->children);

    for (size_t idx_ = block->children.info.count; idx_ > 0; idx_--) {
        node_ptr_assert_no_null(&block->children);
        assert(idx_ < 100000 && "underflow has occured");
        size_t idx = idx_ - 1;
        Node* curr_node = node_ptr_vec_at(&block->children, idx);
        if (curr_node->type == NODE_OPERATOR) {
            flatten_operation_if_nessessary(&block->children, &idx, node_unwrap_operator(curr_node));
        } else if (curr_node->type == NODE_ASSIGNMENT) {
            Node* rhs = node_unwrap_assignment(curr_node)->rhs;
            if (rhs->type == NODE_OPERATOR) {
                node_unwrap_assignment(curr_node)->rhs = node_wrap(
                    move_operator_back(&block->children, &idx, node_unwrap_operator(rhs))
                );
                assert(node_unwrap_assignment(curr_node)->rhs);
            }
        } else if (curr_node->type == NODE_VARIABLE_DEFINITION) {
            Node** prev_node_ref = idx > 0 ? (node_ptr_vec_at_ref(&block->children, idx - 1)) : NULL;
            if (prev_node_ref && (*prev_node_ref)->type != NODE_VARIABLE_DEFINITION) {
                // move curr_node back one
                Node** curr_node_ref = node_ptr_vec_at_ref(&block->children, idx);
                swap_nodes(curr_node_ref, prev_node_ref);
            }
        } else if (curr_node->type == NODE_RETURN_STATEMENT) {
            Node_return_statement* rtn_statement = node_unwrap_return_statement(curr_node);
            if (rtn_statement->child->type == NODE_OPERATOR) {
                rtn_statement->child = node_wrap(move_operator_back(
                    &block->children,
                    &idx,
                    node_unwrap_operator(rtn_statement->child)
                ));
                assert(rtn_statement->child);
            }
        }
    }

    return false;
}

