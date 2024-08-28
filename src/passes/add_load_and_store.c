#include "../node.h"
#include "../nodes.h"
#include "../symbol_table.h"
#include "../parser_utils.h"

static Node_id alloca_new(Node_id var_def) {
    Node_id alloca = node_new();
    nodes_at(alloca)->type = NODE_ALLOCA;
    nodes_at(alloca)->name = nodes_at(var_def)->name;
    return alloca;
}

static Node_id store_new(Node_id var_def) {
    Node_id store = node_new();
    nodes_at(store)->type = NODE_STORE;
    nodes_at(store)->name = nodes_at(var_def)->name;
    return store;
}

static void insert_alloca_and_store(Node_id var_def) {
    Node_id var_def_parent = nodes_at(var_def)->parent;
    Node_id block_parent;
    Node_id fun_def;
    Node_id block_start;
    Node_id new_alloca;
    Node_id new_store;

    switch (nodes_at(var_def_parent)->type) {
        case NODE_FUNCTION_PARAMETERS:
            // fallthrough
        case NODE_ASSIGNMENT: {
            Node_id fun_def_or_decl_or_block = nodes_at(var_def_parent)->parent;
            switch (nodes_at(fun_def_or_decl_or_block)->type) {
                case NODE_FUNCTION_DEFINITION: {
                    block_parent = fun_def_or_decl_or_block;
                    fun_def      = block_parent;
                    block_start  = nodes_get_child_of_type(fun_def, NODE_BLOCK);
                    new_alloca   = alloca_new(var_def);
                    new_store    = store_new(var_def);

                }
                    break;
                case NODE_FUNCTION_DECLARATION:
                    // no need to alloca variable for extern declaration
                    return;
                case NODE_BLOCK: {
                    new_alloca = alloca_new(var_def);
                    new_store = store_new(var_def);
                    block_start = fun_def_or_decl_or_block;
                    break;
                }
                default:
                    unreachable();
            }
            break;
        }
        default:
           todo();
    }

    log_tree(LOG_DEBUG, 0);
    Node_id last_alloca;
    if (nodes_try_get_last_child_of_type(&last_alloca, block_start, NODE_ALLOCA)) {
        nodes_insert_after(last_alloca, new_alloca);
    } else {
        last_alloca = nodes_get_child(block_start, 0);
        nodes_insert_before(last_alloca, new_alloca);
    }

    Node_id last_store;
    if (nodes_try_get_last_child_of_type(&last_store, block_start, NODE_STORE)) {
        nodes_insert_after(last_store, new_store);
    } else {
        last_store = nodes_get_child(block_start, 0);
        nodes_insert_after(last_store, new_store);
    }
}

//static void load_function_call(Node_id)

void add_load_and_store(Node_id curr_node) {
    log_tree(LOG_DEBUG, 0);
    log_tree(LOG_DEBUG, curr_node);
    switch (nodes_at(curr_node)->type) {
        case NODE_LITERAL:
            return;
        case NODE_FUNCTION_CALL:
            return;
        case NODE_FUNCTION_DEFINITION:
            return;
        case NODE_FUNCTION_PARAMETERS:
            return;
        case NODE_FUNCTION_RETURN_TYPES:
            return;
        case NODE_LANG_TYPE:
            return;
        case NODE_OPERATOR:
            todo();
        case NODE_BLOCK:
            return;
        case NODE_SYMBOL:
            return;
        case NODE_RETURN_STATEMENT:
            return;
        case NODE_VARIABLE_DEFINITION:
            insert_alloca_and_store(curr_node);
            return;
        case NODE_FUNCTION_DECLARATION:
            return;
        case NODE_ASSIGNMENT:
            return;
        case NODE_FOR_LOOP:
            todo();
        case NODE_FOR_VARIABLE_DEF:
            todo();
        case NODE_FOR_LOWER_BOUND:
            todo();
        case NODE_FOR_UPPER_BOUND:
            todo();
        case NODE_GOTO:
            return;
        case NODE_COND_GOTO:
            return;
        case NODE_NO_TYPE:
            todo();
        case NODE_LABEL:
            return;
        case NODE_ALLOCA:
            return;
        case NODE_STORE:
            return;
        case NODE_LOAD:
            return;
        default:
            todo();
    }
}
