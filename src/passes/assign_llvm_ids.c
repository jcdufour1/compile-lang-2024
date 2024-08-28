#include "../node.h"
#include "../nodes.h"
#include "../symbol_table.h"
#include "../parser_utils.h"

void assign_llvm_ids(Node_id curr_node) {
    static size_t llvm_id_for_next_var = 1;
    log_tree(LOG_DEBUG, 0);
    log_tree(LOG_DEBUG, curr_node);

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
        case NODE_FUNCTION_CALL:
            nodes_at(curr_node)->llvm_id = llvm_id_for_next_var;
            llvm_id_for_next_var++;
            return;
        case NODE_FUNCTION_RETURN_TYPES:
            return;
        case NODE_RETURN_STATEMENT:
            return;
        case NODE_LANG_TYPE:
            return;
        default:
            unreachable();
    }

    todo();
}
