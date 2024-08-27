
#include "../node.h"
#include "../nodes.h"
#include "../util.h"

void walk_tree(Node_id root, void (callback)(Node_id curr_node)) {
    if (root == NODE_IDX_NULL) {
        return;
    }

    callback(root);
    nodes_foreach_child(child, root) {
        walk_tree(child, callback);
    }
    walk_tree(nodes_at(root)->left_child, callback);
}
