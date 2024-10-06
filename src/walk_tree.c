
#include "node.h"
#include "nodes.h"
#include "util.h"

bool walk_tree(Node* input_node, int recursion_depth, bool (callback)(Node* input_node, int recursion_depth)) {
    if (!input_node) {
        return false;
    }

    nodes_foreach_from_curr(curr_node, input_node) {
        callback(curr_node, recursion_depth);

        switch (curr_node->type) {
            case NODE_FUNCTION_PARAMETERS:
                walk_tree(node_wrap(node_unwrap_function_params(curr_node)->child), recursion_depth, callback);
                break;
            case NODE_FUNCTION_RETURN_TYPES:
                walk_tree(node_wrap(node_unwrap_function_return_types(curr_node)->child), recursion_depth, callback);
                break;
            case NODE_ASSIGNMENT:
                walk_tree(node_wrap(node_unwrap_assignment(curr_node)->lhs), recursion_depth, callback);
                walk_tree(node_wrap(node_unwrap_assignment(curr_node)->rhs), recursion_depth, callback);
                break;
            case NODE_SYMBOL_TYPED:
                break;
            case NODE_SYMBOL_UNTYPED:
                break;
            default:
                walk_tree(get_left_child(curr_node), recursion_depth + 1, callback);
                break;
        }
    }
    return false;
}
