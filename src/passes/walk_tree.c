
#include "../node.h"
#include "../nodes.h"
#include "../util.h"

void walk_tree(Node_id root, void (callback)(Node_id curr_node)) {
    if (node_is_null(root)) {
        return;
    }

    callback(root);
    nodes_foreach_child(child, root) {
        walk_tree(child, callback);
    }
}
