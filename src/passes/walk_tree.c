
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
bool walk_tree(Node* curr_node, bool (callback)(Node* curr_node)) {
    log(LOG_DEBUG, "walk tree: "NODE_FMT"\n", node_print(curr_node));
    if (!curr_node) {
        return false;
    }

    // this is for incase curr_node is modified in callback
    LOCAL_STATUS status;
    Node* prev = curr_node->prev;
    Node* parent = curr_node->parent;
    if (!parent) {
        // we are at the actual root
        status = LOCAL_CURR_IS_ROOT;
    } else if (!prev) {
        status = LOCAL_PARENT;
    } else {
        status = LOCAL_PREV;
        assert(prev);
    }

    bool delete_curr_node = callback(curr_node);

    // this is for incase curr_node is modified in callback
    switch (status) {
        case LOCAL_CURR_IS_ROOT:
            // assuming that actual root is 0
            curr_node = root_of_tree;
            break;
        case LOCAL_PARENT:
            curr_node = parent->left_child;
            break;
        case LOCAL_PREV:
            curr_node = prev->next;
            break;
        default:
            unreachable("");
    }
    assert(curr_node);

    Node* child = curr_node->left_child;
    while (child) {
        Node* child_next = child->next;
        bool remove_child = walk_tree(child, callback);
        if (remove_child) {
            nodes_remove(child, true);
            child = child_next;
        } else {
            child = child->next;
        }
    }

    return delete_curr_node;
}
