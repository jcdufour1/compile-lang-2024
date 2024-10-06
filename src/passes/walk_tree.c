
#include "../node.h"
#include "../nodes.h"
#include "../util.h"
#include "passes.h"

bool walk_tree(Node* input_node, bool (callback)(Node* input_node)) {
    if (!input_node) {
        return false;
    }

    nodes_foreach_from_curr(curr_node, input_node) {
        callback(curr_node);

        switch (curr_node->type) {
            case NODE_FUNCTION_PARAMETERS:
                walk_tree(node_wrap(node_unwrap_function_params(curr_node)->child), callback);
                break;
            case NODE_FUNCTION_RETURN_TYPES:
                walk_tree(node_wrap(node_unwrap_function_return_types(curr_node)->child), callback);
                break;
            case NODE_ASSIGNMENT:
                walk_tree(node_wrap(node_unwrap_assignment(curr_node)->lhs), callback);
                walk_tree(node_wrap(node_unwrap_assignment(curr_node)->rhs), callback);
                break;
            default:
                walk_tree(get_left_child(curr_node), callback);
                break;
        }
    }
    return false;
}
