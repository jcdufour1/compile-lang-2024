
#include "../node.h"
#include "../nodes.h"
#include "../util.h"
#include "passes.h"

// callback returns true to indicate that the curr_node should be removed
// nodes (other than decendents of curr_node) should not be removed within the callback
bool walk_tree(Node_id root, bool (callback)(Node_id curr_node)) {
    if (node_is_null(root)) {
        return false;
    }

    bool delete_curr_node = callback(root);

    Node_id child = nodes_left_child(root);
    while (!node_is_null(child)) {
        Node_id child_next = nodes_next(child);
        bool remove_child = walk_tree(child, callback);
        if (remove_child) {
            nodes_remove(child, true);
            child = child_next;
        } else {
            child = nodes_next(child);
        }
    }

    return delete_curr_node;
}
