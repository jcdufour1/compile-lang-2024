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
        Node_operator* child = node_unwrap_operation(operand);
        if (child->type == NODE_OP_UNARY) {
            todo();
        } else if (child->type == NODE_OP_BINARY) {
            Node* operator_sym = node_new(operand->pos, NODE_LLVM_REGISTER_SYM);

            insert_into_node_ptr_vec(
                block_children,
                idx_to_insert_before,
                *idx_to_insert_before,
                operand
            );
            node_unwrap_llvm_register_sym(operator_sym)->node_src = operand;
            assert(node_unwrap_llvm_register_sym(operator_sym)->node_src);
            return operator_sym;
        } else {
            unreachable("");
        }
    } else if (operand->type == NODE_FUNCTION_CALL) {
        Node_llvm_register_sym* fun_sym = node_unwrap_llvm_register_sym(node_new(operand->pos, NODE_LLVM_REGISTER_SYM));
        insert_into_node_ptr_vec(
            block_children,
            idx_to_insert_before,
            *idx_to_insert_before,
            operand
        );
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
    if (old_operation->type == NODE_OP_UNARY) {
        Node_unary* old_unary_op = node_unwrap_op_unary(old_operation);
        old_unary_op->child = flatten_operation_operand(block_children, idx_to_insert_before, old_unary_op->child);
    } else if (old_operation->type == NODE_OP_BINARY) {
        Node_binary* old_bin_op = node_unwrap_op_binary(old_operation);
        old_bin_op->lhs = flatten_operation_operand(block_children, idx_to_insert_before, old_bin_op->lhs);
        old_bin_op->rhs = flatten_operation_operand(block_children, idx_to_insert_before, old_bin_op->rhs);
    } else {
        unreachable("");
    }
}

// operator symbol is returned
static Node_llvm_register_sym* move_operator_back(
    Node_ptr_vec* block_children,
    size_t* idx_to_insert_before,
    Node_operator* operation
) {
    Node_llvm_register_sym* operator_sym = node_unwrap_llvm_register_sym(node_new(node_wrap(operation)->pos, NODE_LLVM_REGISTER_SYM));
    operator_sym->node_src = node_wrap(operation);
    try(try_set_operation_lang_type(operation));
    operator_sym->lang_type = get_operator_lang_type(operation);
    assert(operator_sym->lang_type.str.count > 0);
    assert(operation);
    insert_into_node_ptr_vec(
        block_children,
        idx_to_insert_before,
        *idx_to_insert_before,
        node_wrap(operation)
    );
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

    for (size_t idx = block->children.info.count - 1;; idx--) {
        Node* curr_node = node_ptr_vec_at(&block->children, idx);

        if (curr_node->type == NODE_OPERATOR) {
            flatten_operation_if_nessessary(&block->children, &idx, node_unwrap_operation(curr_node));
        } else if (curr_node->type == NODE_ASSIGNMENT) {
            Node* rhs = node_unwrap_assignment(curr_node)->rhs;
            if (rhs->type == NODE_OPERATOR) {
                node_unwrap_assignment(curr_node)->rhs = node_wrap(
                    move_operator_back(&block->children, &idx, node_unwrap_operation(rhs))
                );
                assert(node_unwrap_assignment(curr_node)->rhs);
            }
        } else if (curr_node->type == NODE_RETURN_STATEMENT) {
            Node_return_statement* rtn_statement = node_unwrap_return_statement(curr_node);
            if (rtn_statement->child->type == NODE_OPERATOR) {
                rtn_statement->child = node_wrap(move_operator_back(
                    &block->children,
                    &idx,
                    node_unwrap_operation(rtn_statement->child)
                ));
                assert(rtn_statement->child);
            }
        }
        
        if (idx < 1) {
            break;
        }
    }

    return false;
}

