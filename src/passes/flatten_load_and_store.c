#include "passes.h"
#include "../node.h"
#include "../nodes.h"

static void do_store(Node* node_to_insert_before, Node* store) {
    Node* src = nodes_single_child(store);
    switch (src->type) {
        case NODE_FUNCTION_CALL: {
            nodes_remove(src, true);
            nodes_insert_before(node_to_insert_before, src);
            Node* new_src = node_new();
            new_src->type = NODE_FUNCTION_RETURN_VALUE_SYM;
            new_src->node_to_load = src;
            nodes_append_child(store, new_src);
            break;
        }
        case NODE_OPERATOR: {
            nodes_remove(src, true);
            nodes_insert_before(node_to_insert_before, src);
            Node* new_src = node_new();
            new_src->type = NODE_OPERATOR_RETURN_VALUE_SYM;
            new_src->node_to_load = src;
            nodes_append_child(store, new_src);
            break;
        }
        case NODE_STRUCT_LITERAL:
            store->type = NODE_MEMCPY;
            break;
        case NODE_FUNCTION_PARAM_SYM:
            break;
        case NODE_FUNCTION_RETURN_VALUE_SYM:
            break;
        case NODE_OPERATOR_RETURN_VALUE_SYM:
            break;
        case NODE_LITERAL:
            break;
        case NODE_SYMBOL:
            break;
        default:
            unreachable(NODE_FMT"\n", node_print(src));
    }
}

bool flatten_load_and_store(Node* start_start_node) {
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
            case NODE_NO_TYPE:
                break;
            case NODE_STRUCT_DEFINITION:
                break;
            case NODE_STRUCT_LITERAL:
                break;
            case NODE_STRUCT_MEMBER_SYM:
                break;
            case NODE_BLOCK:
                break;
            case NODE_FUNCTION_DEFINITION:
                flatten_load_and_store(nodes_get_child_of_type(curr_node, NODE_BLOCK));
                break;
            case NODE_FUNCTION_PARAMETERS:
                break;
            case NODE_FUNCTION_RETURN_TYPES:
                break;
            case NODE_FUNCTION_CALL:
                break;
            case NODE_FUNCTION_PARAM_SYM:
                break;
            case NODE_LITERAL:
                break;
            case NODE_LANG_TYPE:
                break;
            case NODE_OPERATOR:
                break;
            case NODE_SYMBOL:
                break;
            case NODE_RETURN_STATEMENT:
                break;
            case NODE_VARIABLE_DEFINITION:
                break;
            case NODE_FUNCTION_DECLARATION:
                break;
            case NODE_FOR_LOOP:
                break;
            case NODE_FOR_LOWER_BOUND:
                break;
            case NODE_FOR_UPPER_BOUND:
                break;
            case NODE_FOR_VARIABLE_DEF:
                break;
            case NODE_IF_STATEMENT:
                break;
            case NODE_IF_CONDITION:
                break;
            case NODE_ASSIGNMENT:
                break;
            case NODE_GOTO:
                break;
            case NODE_COND_GOTO:
                break;
            case NODE_LABEL:
                break;
            case NODE_ALLOCA:
                break;
            case NODE_LOAD:
                break;
            case NODE_LOAD_STRUCT_MEMBER:
                break;
            case NODE_STORE:
                do_store(curr_node, curr_node);
                break;
            case NODE_STORE_STRUCT_MEMBER:
                break;
            case NODE_MEMCPY:
                break;
            case NODE_FUNCTION_RETURN_VALUE_SYM:
                break;
            case NODE_OPERATOR_RETURN_VALUE_SYM:
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

