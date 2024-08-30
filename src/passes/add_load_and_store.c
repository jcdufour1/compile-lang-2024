#include "../node.h"
#include "../nodes.h"
#include "../symbol_table.h"
#include "../parser_utils.h"

#include "passes.h"

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

static void insert_alloca(Node_id var_def) {
    Node_id new_alloca = alloca_new(var_def);

    log_tree(LOG_DEBUG, node_id_from(0));

    Node_id node_to_insert_after = node_id_from(NODE_IDX_NULL);
    nodes_foreach_from_curr_rev(curr, var_def) {
        if (nodes_at(var_def)->type != NODE_ALLOCA) {
            node_to_insert_after = curr;
            continue;
        }
    }

    assert(!node_is_null(node_to_insert_after));

    nodes_insert_after(node_to_insert_after, new_alloca);
}

static void insert_load(Node_id node_insert_load_before, Node_id symbol_call) {
    switch (nodes_at(symbol_call)->type) {
        case NODE_LITERAL:
            // fallthrough
        case NODE_SYMBOL:
            // fallthrough
        case NODE_VARIABLE_DEFINITION:
            log_tree(LOG_DEBUG, symbol_call);
            node_printf(symbol_call);
            assert(nodes_at(symbol_call)->name.count > 0);
            break;
        default:
            node_printf(symbol_call);
            todo();
    }

    Node_id load = node_new();
    nodes_at(load)->type = NODE_LOAD;
    nodes_at(load)->name = nodes_at(symbol_call)->name;
    nodes_insert_before(node_insert_load_before, load);
}

static void insert_store_assignment(Node_id assignment, Node_id item_to_store) {
    assert(nodes_at(assignment)->type == NODE_ASSIGNMENT);

    Node_id lhs = nodes_get_child(assignment, 0);

    log_tree(LOG_DEBUG, node_id_from(0));
    switch (nodes_at(lhs)->type) {
        case NODE_VARIABLE_DEFINITION:
            nodes_remove(lhs, false);
            nodes_insert_before(assignment, lhs);
            insert_alloca(lhs);
            // fallthrough
        case NODE_SYMBOL:
            node_printf(lhs);
            break;
        default:
            todo();
    }

    Node_id new_store = node_new();
    node_printf(lhs);
    nodes_at(new_store)->type = NODE_STORE;
    node_printf(lhs);
    nodes_at(new_store)->name = nodes_at(lhs)->name;
    node_printf(lhs);
    node_printf(new_store);
    log_tree(LOG_DEBUG, node_id_from(0));
    nodes_remove(item_to_store, false);
    log_tree(LOG_DEBUG, node_id_from(0));
    nodes_append_child(new_store, item_to_store);
    log_tree(LOG_DEBUG, node_id_from(0));

    nodes_insert_before(assignment, new_store);
    log_tree(LOG_DEBUG, node_id_from(0));
    log_tree(LOG_DEBUG, node_id_from(0));
}

//static void load_function_call(Node_id)

static void add_load_foreach_arg(Node_id function_call) {
    nodes_foreach_child(argument, function_call) {
        insert_load(function_call, argument);
    }
}

static void add_load_return_statement(Node_id return_statement) {
    switch (nodes_at(nodes_single_child(return_statement))->type) {
        case NODE_SYMBOL:
            insert_load(return_statement, nodes_single_child(return_statement));
            return;
        case NODE_LITERAL:
            return;
        default:
            unreachable("");
    }
}

static void add_load_cond_goto(Node_id cond_goto) {
    Node_id operator = nodes_get_child(cond_goto, 0);
    Node_id lhs = nodes_get_child(operator, 0);
    Node_id rhs = nodes_get_child(operator, 1);

    switch (nodes_at(lhs)->type) {
        case NODE_SYMBOL:
            insert_load(cond_goto, lhs);
            break;
        default:
            unreachable("");
    }

    if (nodes_at(rhs)->type != NODE_LITERAL) {
        todo();
    }
}

static void add_load_operator(Node_id operator) {
    Node_id lhs = nodes_get_child(operator, 0);
    Node_id rhs = nodes_get_child(operator, 1);

    if (nodes_at(lhs)->type != NODE_LITERAL) {
        //insert_load(operator, lhs);
    }

    if (nodes_at(rhs)->type != NODE_LITERAL) {
        //insert_load(operator, rhs);
    }

    log_tree(LOG_DEBUG, node_id_from(0));
    todo();
}

bool add_load_and_store(Node_id curr_node) {
    //log_tree(LOG_DEBUG, 0);
    //log_tree(LOG_DEBUG, curr_node);
    //log(LOG_DEBUG, NODE_FMT"\n", node_print(curr_node));
    switch (nodes_at(curr_node)->type) {
        case NODE_LITERAL:
            return false;
        case NODE_FUNCTION_CALL:
            add_load_foreach_arg(curr_node);
            return false;
        case NODE_FUNCTION_DEFINITION:
            return false;
        case NODE_FUNCTION_PARAMETERS:
            return false;
        case NODE_FUNCTION_RETURN_TYPES:
            return false;
        case NODE_LANG_TYPE:
            return false;
        case NODE_OPERATOR:
            //add_load_operator(curr_node);
            return false;
        case NODE_BLOCK:
            return false;
        case NODE_SYMBOL:
            return false;
        case NODE_RETURN_STATEMENT:
            add_load_return_statement(curr_node);
            return false;
        case NODE_VARIABLE_DEFINITION:
            if (nodes_at(nodes_at(curr_node)->parent)->type == NODE_FUNCTION_PARAMETERS) {
                return false;
            }
            insert_alloca(curr_node);
            return false;
        case NODE_FUNCTION_DECLARATION:
            return false;
        case NODE_ASSIGNMENT:
            insert_store_assignment(curr_node, nodes_get_child(curr_node, 1));
            return true;
        case NODE_FOR_LOOP:
            todo();
        case NODE_FOR_VARIABLE_DEF:
            todo();
        case NODE_FOR_LOWER_BOUND:
            todo();
        case NODE_FOR_UPPER_BOUND:
            todo();
        case NODE_GOTO:
            return false;
        case NODE_COND_GOTO:
            add_load_cond_goto(curr_node);
            return false;
        case NODE_NO_TYPE:
            todo();
        case NODE_LABEL:
            return false;
        case NODE_ALLOCA:
            return false;
        case NODE_STORE:
            return false;
        case NODE_LOAD:
            return false;
        default:
            todo();
    }
}
