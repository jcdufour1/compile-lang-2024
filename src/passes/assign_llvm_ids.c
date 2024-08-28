#include "../node.h"
#include "../nodes.h"
#include "../symbol_table.h"
#include "../parser_utils.h"

void assign_llvm_ids(Node_id curr_node) {
    todo();
    /*
    static size_t llvm_id_for_next_var = 1;

    switch (nodes_at(curr_node)->type) {
        case NODE_FUNCTION_PARAMETERS:
            todo();
            break;
        case NODE_VARIABLE_DEFINITION:
            nodes_at(curr_node)->llvm_id_symbol_def.def = llvm_id_for_next_var;
            break;
        case NODE_SYMBOL: {
            Node_id sym_def;
            try(sym_tbl_lookup(&sym_def, nodes_at(curr_node)->name));
            nodes_at(curr_node)->llvm_id_symbol_call.last_store = nodes_at(sym_def)->llvm_id_symbol_def.last_store;
            break;
        }
        default:
            unreachable();

        llvm_id_for_next_var++;
    }

    */
    todo();
}
