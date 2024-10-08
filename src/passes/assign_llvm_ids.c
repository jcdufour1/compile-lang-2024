#include "../node.h"
#include "../nodes.h"
#include "../symbol_table.h"
#include "../parser_utils.h"
#include "../nodes.h"

bool assign_llvm_ids(Node* curr_node) {
    static size_t llvm_id_for_next_var = 1;

    switch (curr_node->type) {
        case NODE_STRUCT_LITERAL:
            return false;
        case NODE_STRUCT_DEFINITION:
            return false;
        case NODE_FUNCTION_PARAMETERS:
            return false;
        case NODE_SYMBOL_TYPED:
            return false;
        case NODE_LITERAL:
            return false;
        case NODE_BLOCK:
            return false;
        case NODE_FUNCTION_DECLARATION:
            return false;
        case NODE_FUNCTION_DEFINITION:
            return false;
        case NODE_FUNCTION_RETURN_TYPES:
            return false;
        case NODE_RETURN_STATEMENT:
            return false;
        case NODE_LANG_TYPE:
            return false;
        case NODE_SYMBOL_UNTYPED:
            return false;
        case NODE_LOAD_STRUCT_ELEMENT_PTR:
            node_unwrap_load_elem_ptr(curr_node)->llvm_id = llvm_id_for_next_var;
            llvm_id_for_next_var += 2;
            return false;
        case NODE_LOAD_ANOTHER_NODE:
            node_unwrap_load_another_node(curr_node)->llvm_id = llvm_id_for_next_var;
            llvm_id_for_next_var += 2;
            return false;
        case NODE_STORE_ANOTHER_NODE:
            node_unwrap_store_another_node(curr_node)->llvm_id = llvm_id_for_next_var;
            llvm_id_for_next_var += 2;
            return false;
        case NODE_STRUCT_MEMBER_SYM_TYPED:
            node_unwrap_struct_member_sym_typed(curr_node)->llvm_id = llvm_id_for_next_var;
            llvm_id_for_next_var += 2;
            return false;
        case NODE_VARIABLE_DEFINITION:
            node_unwrap_variable_def(curr_node)->llvm_id = llvm_id_for_next_var;
            llvm_id_for_next_var += 2;
            return false;
        case NODE_FUNCTION_CALL:
            node_unwrap_function_call(curr_node)->llvm_id = llvm_id_for_next_var;
            llvm_id_for_next_var += 2;
            return false;
        case NODE_ASSIGNMENT:
            unreachable("");
        case NODE_ALLOCA:
            node_unwrap_alloca(curr_node)->llvm_id = llvm_id_for_next_var;
            llvm_id_for_next_var += 2;
            return false;
        case NODE_GOTO:
            node_unwrap_goto(curr_node)->llvm_id = llvm_id_for_next_var;
            llvm_id_for_next_var += 2;
            return false;
        case NODE_COND_GOTO:
            node_unwrap_cond_goto(curr_node)->llvm_id = llvm_id_for_next_var;
            llvm_id_for_next_var += 2;
            return false;
        case NODE_IF_CONDITION:
            unreachable("");
        case NODE_OPERATOR:
            node_unwrap_operator(curr_node)->llvm_id = llvm_id_for_next_var;
            llvm_id_for_next_var += 2;
            return false;
        case NODE_LABEL:
            node_unwrap_label(curr_node)->llvm_id = llvm_id_for_next_var;
            llvm_id_for_next_var += 2;
            return false;
        case NODE_LLVM_STORE_LITERAL:
            node_unwrap_llvm_store_literal(curr_node)->llvm_id = llvm_id_for_next_var;
            llvm_id_for_next_var += 2;
            return false;
        case NODE_LLVM_STORE_STRUCT_LITERAL:
            node_unwrap_llvm_store_struct_literal(curr_node)->llvm_id = llvm_id_for_next_var;
            llvm_id_for_next_var += 2;
            return false;
        case NODE_STRUCT_MEMBER_SYM_PIECE_TYPED:
            unreachable("");
        case NODE_PTR_BYVAL_SYM:
            node_unwrap_ptr_byval_sym(curr_node)->llvm_id = llvm_id_for_next_var;
            llvm_id_for_next_var += 2;
            return false;
        case NODE_LLVM_REGISTER_SYM:
            node_unwrap_llvm_register_sym(curr_node)->llvm_id = llvm_id_for_next_var;
            llvm_id_for_next_var += 2;
            return false;
        default:
            unreachable(NODE_FMT"\n", node_print(curr_node));
    }
}
