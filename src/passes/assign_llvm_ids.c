#include "../node.h"
#include "../nodes.h"
#include "../symbol_table.h"
#include "../parser_utils.h"

void assign_llvm_ids(Node_id curr_node) {
    static size_t llvm_id_for_next_var = 1;
    //log_tree(LOG_DEBUG, 0);
    //log_tree(LOG_DEBUG, curr_node);

    switch (nodes_at(curr_node)->type) {
        case NODE_FUNCTION_PARAMETERS:
            return;
        case NODE_VARIABLE_DEFINITION:
            return;
        case NODE_SYMBOL:
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
        case NODE_FUNCTION_CALL:
            // fallthrough
        case NODE_ASSIGNMENT:
            // fallthrough
        case NODE_ALLOCA:
            // fallthrough
        case NODE_LOAD:
            // fallthrough
        case NODE_GOTO:
            // fallthrough
        case NODE_COND_GOTO:
            // fallthrough
        case NODE_OPERATOR:
            // fallthrough
        case NODE_LABEL:
            // fallthrough
        case NODE_STORE:
            nodes_at(curr_node)->llvm_id = llvm_id_for_next_var;
            llvm_id_for_next_var++;
            return;
        default:
            unreachable();
    }
}
