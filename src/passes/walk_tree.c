
#include "../node.h"
#include "../nodes.h"
#include "../util.h"
#include "passes.h"

bool walk_tree(Node* input_node, bool (callback)(Node* input_node)) {
    nodes_foreach_from_curr(curr_node, input_node) {
        log(LOG_DEBUG, NODE_FMT"\n", node_print(curr_node));
        callback(curr_node);
        if (curr_node->left_child) {
            walk_tree(curr_node->left_child, callback);
        }
    }
    return false;
}
