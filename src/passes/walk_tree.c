
#include "../node.h"
#include "../nodes.h"
#include "../util.h"
#include "passes.h"

typedef enum {
    LOCAL_PARENT,
    LOCAL_PREV,
    LOCAL_CURR_IS_ROOT,
} LOCAL_STATUS;

// callback returns true to indicate that the curr_node should be removed
// nodes (other than decendents of curr_node) should not be removed within the callback
// `nodes_replace` should only be called on curr_node or decendents
bool walk_tree(Node_id curr_node, bool (callback)(Node_id curr_node)) {
    const Node_id original_root = curr_node;

    if (node_is_null(curr_node)) {
        return false;
    }

    nodes_assert_tree_linkage_is_consistant(curr_node);

    // this is for incase curr_node is modified in callback
    LOCAL_STATUS status;
    Node_id prev = nodes_prev(curr_node);
    Node_id parent = nodes_parent(curr_node);
    log_tree(LOG_DEBUG, curr_node);
    if (node_is_null(parent)) {
        log(LOG_DEBUG, "thing 1\n");
        // we are at the actual root
        status = LOCAL_CURR_IS_ROOT;
    } else if (node_is_null(prev)) {
        log(LOG_DEBUG, "thing 2\n");
        status = LOCAL_PARENT;
    } else {
        log(LOG_DEBUG, "thing 3\n");
        status = LOCAL_PREV;
    }

    bool delete_curr_node = callback(curr_node);
    nodes_assert_tree_linkage_is_consistant(node_id_from(0));

    // this is for incase curr_node is modified in callback
    switch (status) {
        case LOCAL_CURR_IS_ROOT:
            log(LOG_DEBUG, "thing 4\n");
            // assuming that actual root is 0
            curr_node = node_id_from(0);
            break;
        case LOCAL_PARENT:
            log(LOG_DEBUG, "thing 5\n");
            curr_node = nodes_left_child(parent);
            break;
        case LOCAL_PREV:
            log(LOG_DEBUG, "thing 6\n");
            log_tree(LOG_DEBUG, prev);
            log_tree(LOG_DEBUG, curr_node);
            curr_node = nodes_next(prev);
            break;
        default:
            unreachable("");
    }
    log_tree(LOG_DEBUG, curr_node);

    nodes_assert_tree_linkage_is_consistant(curr_node);

    Node_id child = nodes_left_child(curr_node);
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
