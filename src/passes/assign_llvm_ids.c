#include "../node.h"
#include "../nodes.h"
#include "../symbol_table.h"
#include "../parser_utils.h"
#include "../nodes.h"
#include "passes.h"

void assign_llvm_ids(Env* env) {
    Node* curr_node = vec_top(&env->ancesters);
    static size_t llvm_id_for_next_var = 1;

    switch (curr_node->type) {
        case NODE_STRUCT_LITERAL:
            return;
        case NODE_STRUCT_DEF:
            return;
        case NODE_FUNCTION_PARAMS:
            return;
        case NODE_SYMBOL_TYPED:
            return;
        case NODE_LITERAL:
            return;
        case NODE_BLOCK:
            return;
        case NODE_FUNCTION_DECLARATION:
            return;
        case NODE_FUNCTION_DEFINITION:
            return;
        case NODE_FUNCTION_RETURN_TYPES:
            return;
        case NODE_RETURN_STATEMENT:
            return;
        case NODE_LANG_TYPE:
            return;
        case NODE_SYMBOL_UNTYPED:
            return;
        case NODE_LOAD_ELEMENT_PTR:
            node_unwrap_load_element_ptr(curr_node)->llvm_id = llvm_id_for_next_var;
            llvm_id_for_next_var += 2;
            return;
        case NODE_LOAD_ANOTHER_NODE:
            node_unwrap_load_another_node(curr_node)->llvm_id = llvm_id_for_next_var;
            llvm_id_for_next_var += 2;
            return;
        case NODE_STORE_ANOTHER_NODE:
            node_unwrap_store_another_node(curr_node)->llvm_id = llvm_id_for_next_var;
            llvm_id_for_next_var += 2;
            return;
        case NODE_STRUCT_MEMBER_SYM_TYPED:
            node_unwrap_struct_member_sym_typed(curr_node)->llvm_id = llvm_id_for_next_var;
            llvm_id_for_next_var += 2;
            return;
        case NODE_VARIABLE_DEF:
            node_unwrap_variable_def(curr_node)->llvm_id = llvm_id_for_next_var;
            llvm_id_for_next_var += 2;
            return;
        case NODE_FUNCTION_CALL:
            node_unwrap_function_call(curr_node)->llvm_id = llvm_id_for_next_var;
            llvm_id_for_next_var += 2;
            return;
        case NODE_ASSIGNMENT:
            log_tree(LOG_ERROR, curr_node);
            unreachable("");
        case NODE_ALLOCA:
            node_unwrap_alloca(curr_node)->llvm_id = llvm_id_for_next_var;
            llvm_id_for_next_var += 2;
            return;
        case NODE_GOTO:
            node_unwrap_goto(curr_node)->llvm_id = llvm_id_for_next_var;
            llvm_id_for_next_var += 2;
            return;
        case NODE_COND_GOTO:
            node_unwrap_cond_goto(curr_node)->llvm_id = llvm_id_for_next_var;
            llvm_id_for_next_var += 2;
            return;
        case NODE_IF_CONDITION:
            unreachable("");
        case NODE_OPERATOR: {
            Node_operator* operator = node_unwrap_operator(curr_node);
            if (operator->type == NODE_OP_UNARY) {
                node_unwrap_op_unary(operator)->llvm_id = llvm_id_for_next_var;
                llvm_id_for_next_var += 2;
                return;
            } else if (operator->type == NODE_OP_BINARY) {
                node_unwrap_op_binary(operator)->llvm_id = llvm_id_for_next_var;
                llvm_id_for_next_var += 2;
                return;
            } else {
                unreachable("");
            }
        }
        break;
        case NODE_LABEL:
            node_unwrap_label(curr_node)->llvm_id = llvm_id_for_next_var;
            llvm_id_for_next_var += 2;
            return;
        case NODE_LLVM_STORE_LITERAL:
            node_unwrap_llvm_store_literal(curr_node)->llvm_id = llvm_id_for_next_var;
            llvm_id_for_next_var += 2;
            return;
        case NODE_LLVM_STORE_STRUCT_LITERAL:
            node_unwrap_llvm_store_struct_literal(curr_node)->llvm_id = llvm_id_for_next_var;
            llvm_id_for_next_var += 2;
            return;
        case NODE_STRUCT_MEMBER_SYM_PIECE_TYPED:
            unreachable("");
        case NODE_PTR_BYVAL_SYM:
            node_unwrap_ptr_byval_sym(curr_node)->llvm_id = llvm_id_for_next_var;
            llvm_id_for_next_var += 2;
            return;
        case NODE_LLVM_REGISTER_SYM:
            node_unwrap_llvm_register_sym(curr_node)->llvm_id = llvm_id_for_next_var;
            llvm_id_for_next_var += 2;
            return;
        default:
            unreachable(NODE_FMT"\n", node_print(curr_node));
    }
}
