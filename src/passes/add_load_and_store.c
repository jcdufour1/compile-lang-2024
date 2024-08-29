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

static Node_id store_new(Node_id item_to_store) {
    (void) item_to_store;
    todo();
    Node_id store = node_new();
    nodes_at(store)->type = NODE_STORE;
    //nodes_at(store)->name = nodes_at(var_def)->name;
    return store;
}

static void insert_store_assignment(Node_id assignment, Node_id item_to_store) {
    Node_id lhs = nodes_get_child(assignment, 0);
    Node_id new_store = node_new();
    nodes_at(new_store)->type = NODE_STORE;
    nodes_at(new_store)->name = nodes_at(lhs)->name;
    nodes_reset_links(item_to_store);
    nodes_append_child(new_store, item_to_store);

    nodes_insert_before(assignment, new_store);
}

static void insert_alloca(Node_id var_def) {
    Node_id var_def_parent = nodes_at(var_def)->parent;

    Node_id new_alloca = alloca_new(var_def);

    log_tree(LOG_DEBUG, 0);
    Node_id last_alloca;

    Node_id node_to_insert_after;
    nodes_foreach_from_curr_rev(curr, var_def) {
        if (nodes_at(var_def)->type != NODE_ALLOCA) {
            node_to_insert_after = curr;
            continue;
        }
    }

    nodes_insert_after(node_to_insert_after, new_alloca);
}

//static void load_function_call(Node_id)

static void add_load(Node_id node_insert_load_before, Node_id symbol_call) {
    Node_id load = node_new();
    nodes_at(load)->type = NODE_LOAD;
    nodes_at(load)->name = nodes_at(symbol_call)->name;
    nodes_insert_before(node_insert_load_before, load);
}

static void add_load_foreach_arg(Node_id function_call) {
    nodes_foreach_child(argument, function_call) {
        add_load(function_call, argument);
    }
}

static void add_load_return_statement(Node_id return_statement) {
    switch (nodes_at(nodes_single_child(return_statement))->type) {
        case NODE_SYMBOL:
            add_load(return_statement, nodes_single_child(return_statement));
            return;
        case NODE_LITERAL:
            return;
        default:
            unreachable();
    }
}

void add_load_and_store(Node_id curr_node) {
    //log_tree(LOG_DEBUG, 0);
    //log_tree(LOG_DEBUG, curr_node);
    //log(LOG_DEBUG, NODE_FMT"\n", node_print(curr_node));
    switch (nodes_at(curr_node)->type) {
        case NODE_LITERAL:
            return;
        case NODE_FUNCTION_CALL:
            add_load_foreach_arg(curr_node);
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
            add_load_return_statement(curr_node);
            return;
        case NODE_VARIABLE_DEFINITION:
            if (nodes_at(nodes_at(curr_node)->parent)->type == NODE_FUNCTION_PARAMETERS) {
                return;
            }
            insert_alloca(curr_node);
            return;
        case NODE_FUNCTION_DECLARATION:
            return;
        case NODE_ASSIGNMENT:
            insert_store_assignment(curr_node, nodes_get_child(curr_node, 1));
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
