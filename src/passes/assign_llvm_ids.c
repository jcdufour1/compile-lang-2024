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
        case NODE_FUNCTION_PARAM_SYM:
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
            if (!curr_node->parent || curr_node->parent->type != NODE_COND_GOTO) {
                unreachable("node_symbol_untyped should not exist here except in conditional goto");
            }
            // fallthrough
        case NODE_LOAD_STRUCT_ELEMENT_PTR:
            // fallthrough
        case NODE_LOAD_ANOTHER_NODE:
            // fallthrough
        case NODE_STORE_ANOTHER_NODE:
            // fallthrough
        case NODE_LOAD_STRUCT_MEMBER:
            // fallthrough
        case NODE_STORE_STRUCT_MEMBER:
            // fallthrough
        case NODE_STRUCT_MEMBER_SYM:
            // fallthrough
        case NODE_VARIABLE_DEFINITION:
            // fallthrough
        case NODE_FUNCTION_CALL:
            // fallthrough
        case NODE_ASSIGNMENT:
            // fallthrough
        case NODE_ALLOCA:
            // fallthrough
        case NODE_LOAD_VARIABLE:
            // fallthrough
        case NODE_GOTO:
            // fallthrough
        case NODE_COND_GOTO:
            // fallthrough
        case NODE_OPERATOR:
            // fallthrough
        case NODE_LABEL:
            // fallthrough
        case NODE_FUNCTION_RETURN_VALUE_SYM:
            // fallthrough
        case NODE_OPERATOR_RETURN_VALUE_SYM:
            // fallthrough
        case NODE_STORE_VARIABLE:
            // fallthrough
        case NODE_LLVM_STORE_LITERAL:
            // fallthrough
        case NODE_LLVM_STORE_STRUCT_LITERAL:
            // fallthrough
        case NODE_STRUCT_MEMBER_SYM_PIECE:
            // fallthrough
            curr_node->llvm_id = llvm_id_for_next_var;
            llvm_id_for_next_var += 2;
            return false;
        default:
            unreachable(NODE_FMT"\n", node_print(curr_node));
    }
}
