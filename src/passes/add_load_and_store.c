#include "../node.h"
#include "../nodes.h"
#include "../symbol_table.h"
#include "../parser_utils.h"

#include "passes.h"

static Node* alloca_new(const Node* var_def) {
    Node* alloca = node_new();
    alloca->type = NODE_ALLOCA;
    alloca->name = var_def->name;
    if (is_struct_variable_definition(var_def)) {
        alloca->is_struct_associated = true;
    }
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
        case NODE_STRUCT_LITERAL:
            // fallthrough
        case NODE_LITERAL:
            return;
        case NODE_STRUCT_MEMBER_CALL:
            // fallthrough
        case NODE_STRUCT_ELEMENT_PTR_CALL:
            // fallthrough
        case NODE_SYMBOL:
            // fallthrough
        case NODE_VARIABLE_DEFINITION:
            break;
        default:
            node_printf(symbol_call);
            todo();
    }

    Node* load = node_new();
    if (symbol_call->type == NODE_STRUCT_MEMBER_CALL) {
        load->type = NODE_LOAD_STRUCT_MEMBER;
    } else if (symbol_call->type == NODE_STRUCT_ELEMENT_PTR_CALL) {
        load->type = NODE_LOAD_STRUCT_ELEMENT_PTR;
        log_tree(LOG_DEBUG, symbol_call);
        //load->lang_type = get_member_def(struct_def, )->lang_type;
        todo();
    } else {
        load->type = NODE_LOAD;
    }
    load->name = symbol_call->name;
    log(LOG_DEBUG, "-------\n");
    log_tree(LOG_DEBUG, node_insert_load_before->parent);
    log(LOG_DEBUG, "-------\n");
    nodes_insert_before(node_insert_load_before, load);
    log(LOG_DEBUG, "-------\n");
    log_tree(LOG_DEBUG, node_insert_load_before->parent);
    log(LOG_DEBUG, "-------\n");
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
            unreachable("nested operators should not still be present at this point");
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

static void add_load_foreach_arg(Node* node_insert_before, Node* function_call) {
    log(LOG_DEBUG, "add_foreach_arg\n");
    nodes_foreach_child(argument, function_call) {
        node_printf(argument);
        insert_load(node_insert_before, argument);
    }
}

static void insert_store_assignment(Node* assignment) {
    log(LOG_DEBUG, "THING 76: insert_store_assignment\n");
    assert(assignment->type == NODE_ASSIGNMENT);

    Node* lhs = nodes_get_child(assignment, 0);
    Node* rhs = nodes_get_child(assignment, 1);

    log_tree(LOG_DEBUG, root_of_tree);
    switch (lhs->type) {
        case NODE_VARIABLE_DEFINITION:
            // fallthrough
        case NODE_STRUCT_ELEMENT_PTR_DEF:
            nodes_remove(lhs, true);
            nodes_insert_before(assignment, lhs);
            //insert_alloca(lhs); // TODO: can I remove this?
            break;
        case NODE_SYMBOL:
            node_printf(lhs);
            break;
        case NODE_STRUCT_MEMBER_CALL:
            break;
        case NODE_STRUCT_MEMBER_ELEMENT_PTR_SYMBOL:
            break;
        case NODE_STRUCT_ELEMENT_PTR_CALL:
            break;
        default:
            unreachable(NODE_FMT"\n", node_print(lhs));
    }
   
    switch (rhs->type) {
        case NODE_VARIABLE_DEFINITION:
            todo();
            break;
        case NODE_STRUCT_LITERAL:
            // fallthrough
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
        case NODE_STRUCT_MEMBER_ELEMENT_PTR_SYMBOL:
            break;
        default:
            node_printf(rhs)
            todo();
    }

    Node* store = node_new();
    store->name = lhs->name;
    nodes_remove(rhs, true);
    if (lhs->type == NODE_STRUCT_MEMBER_CALL) {
        store->type = NODE_STORE_STRUCT_MEMBER;
        nodes_append_child(store, node_clone(nodes_single_child(lhs)));
    } else if (lhs->type == NODE_STRUCT_MEMBER_ELEMENT_PTR_SYMBOL) {
        store->type = NODE_STORE_STRUCT_MEMBER;
        nodes_append_child(store, node_clone(nodes_single_child(lhs)));
    } else {
        store->type = NODE_STORE;
    }
    nodes_append_child(store, rhs);

    nodes_insert_before(assignment, store);
}

static void add_load_return_statement(Node* return_statement) {
    assert(return_statement->type == NODE_RETURN_STATEMENT);

    log(LOG_DEBUG, "add_load_return_statement\n");
    log_tree(LOG_DEBUG, return_statement);
    Node* node_to_return = nodes_single_child(return_statement);
    switch (node_to_return->type) {
        case NODE_STRUCT_MEMBER_CALL:
            // fallthrough
        case NODE_SYMBOL:
            // fallthrough
        case NODE_STRUCT_ELEMENT_PTR_CALL:
            insert_load(return_statement, node_to_return);
            return;
        case NODE_FUNCTION_CALL:
            // fallthrough
        case NODE_LITERAL:
            return;
        case NODE_OPERATOR:
            unreachable("operator should not still be the child of return statement at this point");
        default:
            unreachable(NODE_FMT"\n", node_print(node_to_return));
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
        Node* alloca = alloca_new(param);
        alloca->is_fun_param_associated = true;

        nodes_insert_before(fun_block->left_child, alloca);
    }
    log_tree(LOG_DEBUG, fun_params->parent);
}

static void load_function_arguments(Node* fun_call) {
    switch (fun_call->parent->type) {
        case NODE_STORE:
            add_load_foreach_arg(fun_call->parent, fun_call);
            return;
        case NODE_BLOCK:
            add_load_foreach_arg(fun_call, fun_call);
            return;
        case NODE_ASSIGNMENT:
            add_load_foreach_arg(fun_call, fun_call);
            return;
        default:
            unreachable(NODE_FMT"\n", node_print(fun_call->parent));
    }
}

bool add_load_and_store(Node* start_start_node) {
    //log_tree(LOG_DEBUG, 0);
    //log_tree(LOG_DEBUG, curr_node);
    //log(LOG_DEBUG, NODE_FMT"\n", node_print(curr_node));
    if (!start_start_node->left_child) {
        return false;
    }

    Node* curr_node = nodes_get_local_rightmost(start_start_node->left_child);
    while (curr_node) {
        bool go_to_prev = true;
        switch (curr_node->type) {
            case NODE_STRUCT_LITERAL:
                break;
            case NODE_LITERAL:
                break;
            case NODE_FUNCTION_CALL:
                load_function_arguments(curr_node);
                break;
            case NODE_FUNCTION_DEFINITION:
                node_printf(curr_node);
                load_function_parameters(curr_node);
                break;
            case NODE_FUNCTION_PARAMETERS:
                break;
            case NODE_FUNCTION_PARAM_CALL:
                break;
            case NODE_FUNCTION_RETURN_TYPES:
                break;
            case NODE_LANG_TYPE:
                break;
            case NODE_OPERATOR:
                //add_load_operator(curr_node);
                break;
            case NODE_BLOCK:
                break;
            case NODE_STRUCT_ELEMENT_PTR_CALL:
                break;
            case NODE_SYMBOL:
                break;
            case NODE_RETURN_STATEMENT:
                add_load_return_statement(curr_node);
                break;
            case NODE_STRUCT_ELEMENT_PTR_DEF:
                // fallthrough
            case NODE_VARIABLE_DEFINITION:
                if (curr_node->parent->type == NODE_FUNCTION_PARAMETERS ||
                    curr_node->parent->type == NODE_STRUCT_DEFINITION ||
                    curr_node->parent->type == NODE_STRUCT_LITERAL
                ) {
                    break;
                }
                insert_alloca(curr_node);
                break;
            case NODE_FUNCTION_DECLARATION:
                break;
            case NODE_ASSIGNMENT: {
                Node* assignment = curr_node;
                insert_store_assignment(assignment);
                curr_node = assignment->prev;
                nodes_remove(assignment, true);
                go_to_prev = false;
                break;
            }
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
                break;
            case NODE_COND_GOTO:
                add_load_cond_goto(curr_node);
                break;
            case NODE_NO_TYPE:
                log_tree(LOG_DEBUG, curr_node->parent);
                unreachable(NODE_FMT"\n", node_print(curr_node));
            case NODE_LABEL:
                break;
            case NODE_ALLOCA:
                break;
            case NODE_STORE:
                break;
            case NODE_LOAD:
                break;
            case NODE_LOAD_STRUCT_ELEMENT_PTR:
                break;
            case NODE_IF_STATEMENT:
                unreachable("if statement node should not still exist at this point\n");
            case NODE_LOAD_STRUCT_MEMBER:
                break;
            case NODE_STORE_STRUCT_MEMBER:
                break;
            case NODE_STRUCT_MEMBER_CALL:
                break;
            case NODE_STRUCT_MEMBER_ELEMENT_PTR_SYMBOL:
                break;
            case NODE_STRUCT_DEFINITION:
                break;
            default:
                unreachable(NODE_FMT"\n", node_print(curr_node));
        }

        if (go_to_prev) {
            curr_node = curr_node->prev;
        }
    }

    return false;
}
