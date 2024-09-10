#include "../node.h"
#include "../nodes.h"
#include "../symbol_table.h"
#include "../parser_utils.h"

#include "passes.h"

static Node* alloca_new(const Node* var_def) {
    Node* alloca = node_new();
    alloca->type = NODE_ALLOCA;
    alloca->name = var_def->name;
    return alloca;
}

static Node* store_new(Node* item_to_store) {
    (void) item_to_store;
    todo();
    Node* store = node_new();
    store->type = NODE_STORE;
    //store->name = var_def->name;
    return store;
}

static void insert_alloca(Node* var_def) {
    log_tree(LOG_DEBUG, var_def);
    Node* new_alloca = alloca_new(var_def);

    log_tree(LOG_DEBUG, root_of_tree);

    nodes_foreach_from_curr(curr, nodes_get_local_leftmost(var_def)) {
        if (curr->type != NODE_VARIABLE_DEFINITION) {
            if (curr->prev) {
                nodes_insert_after(curr->prev, new_alloca);
                return;
            }
            nodes_insert_before(curr, new_alloca);
            return;
        }
    }

    unreachable("");
}

static void insert_load(Node* node_insert_load_before, Node* symbol_call) {
    log(LOG_DEBUG, "insert_load\n");
    switch (symbol_call->type) {
        case NODE_LITERAL:
            return;
        case NODE_SYMBOL:
            // fallthrough
        case NODE_VARIABLE_DEFINITION:
            log_tree(LOG_DEBUG, symbol_call);
            node_printf(symbol_call);
            assert(symbol_call->name.count > 0);
            break;
        default:
            node_printf(symbol_call);
            todo();
    }

    Node* load = node_new();
    load->type = NODE_LOAD;
    load->name = symbol_call->name;
    nodes_insert_before(node_insert_load_before, load);
}

static void insert_store(Node* node_insert_store_before, Node* symbol_call /* src */) {
    log(LOG_DEBUG, "insert_store\n");
    switch (symbol_call->type) {
        case NODE_LITERAL:
            return;
        case NODE_SYMBOL:
            // fallthrough
        case NODE_VARIABLE_DEFINITION:
            log_tree(LOG_DEBUG, symbol_call);
            node_printf(symbol_call);
            assert(symbol_call->name.count > 0);
            break;
        case NODE_FUNCTION_PARAM_CALL:
            break;
        default:
            node_printf(symbol_call);
            todo();
    }

    Node* store = node_new();
    store->type = NODE_STORE;
    store->name = symbol_call->name;
    nodes_append_child(store, symbol_call);
    nodes_insert_before(node_insert_store_before, store);
}

static void load_operator_operand(Node* node_insert_before, Node* operand) {
    switch (operand->type) {
        case NODE_OPERATOR:
            todo();
        case NODE_LITERAL:
            break;
        case NODE_VARIABLE_DEFINITION:
            todo();
        case NODE_SYMBOL:
            insert_load(node_insert_before, operand);
            break;
        default:
            unreachable("");
    }
}

static void load_operator_operands(Node* node_insert_before, Node* operator) {
    node_printf(operator);
    assert(operator->type == NODE_OPERATOR);
    assert(nodes_count_children(operator) == 2);

    Node* lhs = nodes_get_child(operator, 0);
    Node* rhs = nodes_get_child(operator, 1);

    load_operator_operand(node_insert_before, lhs);
    load_operator_operand(node_insert_before, rhs);
}

static void insert_store_assignment(Node* assignment, Node* item_to_store) {
    log(LOG_DEBUG, "THING 76: insert_store_assignment\n");
    assert(assignment->type == NODE_ASSIGNMENT);

    Node* lhs = nodes_get_child(assignment, 0);
    Node* rhs = nodes_get_child(assignment, 1);

    log_tree(LOG_DEBUG, root_of_tree);
    switch (lhs->type) {
        case NODE_VARIABLE_DEFINITION:
            nodes_remove(lhs, false);
            nodes_insert_before(assignment, lhs);
            insert_alloca(lhs);
            break;
        case NODE_SYMBOL:
            node_printf(lhs);
            break;
        default:
            todo();
    }
   
    switch (rhs->type) {
        case NODE_VARIABLE_DEFINITION:
            todo();
            break;
        case NODE_SYMBOL:
            insert_load(assignment, rhs);
            log_tree(LOG_DEBUG, root_of_tree);
            break;
        case NODE_LITERAL:
            break;
        case NODE_OPERATOR:
            load_operator_operands(assignment, rhs);
            break;
        case NODE_FUNCTION_CALL:
            break;
        default:
            node_printf(rhs)
            todo();
    }

    Node* new_store = node_new();
    new_store->type = NODE_STORE;
    new_store->name = lhs->name;
    nodes_remove(item_to_store, true);
    nodes_append_child(new_store, item_to_store);

    nodes_insert_before(assignment, new_store);
}

//static void load_function_call(Node*)

static void add_load_foreach_arg(Node* function_call) {
    log(LOG_DEBUG, "add_foreach_arg\n");
    nodes_foreach_child(argument, function_call) {
        insert_load(function_call, argument);
    }
}

static void add_load_return_statement(Node* return_statement) {
    log(LOG_DEBUG, "add_load_return_statement\n");
    switch (nodes_single_child(return_statement)->type) {
        case NODE_SYMBOL:
            insert_load(return_statement, nodes_single_child(return_statement));
            return;
        case NODE_LITERAL:
            return;
        default:
            unreachable("");
    }
}

static void add_load_cond_goto(Node* cond_goto) {
    Node* operator = nodes_get_child(cond_goto, 0);
    Node* lhs = nodes_get_child(operator, 0);
    Node* rhs = nodes_get_child(operator, 1);

    insert_load(cond_goto, lhs);
    insert_load(cond_goto, rhs);
}

static void add_load_operator(Node* operator) {
    Node* lhs = nodes_get_child(operator, 0);
    Node* rhs = nodes_get_child(operator, 1);

    if (lhs->type != NODE_LITERAL) {
        //insert_load(operator, lhs);
    }

    if (rhs->type != NODE_LITERAL) {
        //insert_load(operator, rhs);
    }

    log_tree(LOG_DEBUG, root_of_tree);
    todo();
}

static void load_function_parameters(Node* fun_def) {
    assert(fun_def->type == NODE_FUNCTION_DEFINITION);
    Node* fun_params = nodes_get_child_of_type(fun_def, NODE_FUNCTION_PARAMETERS);
    Node* fun_block = nodes_get_child_of_type(fun_def, NODE_BLOCK);

    nodes_foreach_child(param, fun_params) {
        Node* fun_param_call = symbol_new(param->name);
        fun_param_call->type = NODE_FUNCTION_PARAM_CALL;
        insert_store(fun_block->left_child, fun_param_call);

        nodes_insert_before(fun_block->left_child, alloca_new(param));
    }
    log_tree(LOG_DEBUG, fun_params->parent);
}

bool add_load_and_store(Node* curr_node) {
    //log_tree(LOG_DEBUG, 0);
    //log_tree(LOG_DEBUG, curr_node);
    //log(LOG_DEBUG, NODE_FMT"\n", node_print(curr_node));
    switch (curr_node->type) {
        case NODE_LITERAL:
            return false;
        case NODE_FUNCTION_CALL:
            add_load_foreach_arg(curr_node);
            return false;
        case NODE_FUNCTION_DEFINITION:
            node_printf(curr_node);
            load_function_parameters(curr_node);
            return false;
        case NODE_FUNCTION_PARAMETERS:
            return false;
        case NODE_FUNCTION_PARAM_CALL:
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
            if (curr_node->parent->type == NODE_FUNCTION_PARAMETERS) {
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
            unreachable("for loop node should not still exist at this point\n");
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
        case NODE_IF_STATEMENT:
            unreachable("if statement node should not still exist at this point\n");
        default:
            todo();
    }
}
