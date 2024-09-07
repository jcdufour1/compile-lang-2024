
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
    if (!curr_node) {
        return false;
    }

    nodes_assert_tree_linkage_is_consistant(curr_node);

    // this is for incase curr_node is modified in callback
    LOCAL_STATUS status;
    Node* prev = curr_node->prev;
    Node* parent = curr_node->parent;
    //log_tree(LOG_DEBUG, curr_node);
    if (!parent) {
        //log(LOG_DEBUG, "thing 1\n");
        // we are at the actual root
        status = LOCAL_CURR_IS_ROOT;
    } else if (!prev) {
        //log(LOG_DEBUG, "thing 2\n");
        status = LOCAL_PARENT;
    } else {
        //log(LOG_DEBUG, "thing 3\n");
        status = LOCAL_PREV;
    }

    bool delete_curr_node = callback(curr_node);
    nodes_assert_tree_linkage_is_consistant(root_of_tree);
    log(LOG_NOTE, "thkdjfla\n");

    // this is for incase curr_node is modified in callback
    switch (status) {
        case LOCAL_CURR_IS_ROOT:
            //log(LOG_DEBUG, "thing 4\n");
            // assuming that actual root is 0
            curr_node = root_of_tree;
            break;
        case LOCAL_PARENT:
            //log(LOG_DEBUG, "thing 5\n");
            curr_node = parent->left_child;
            break;
        case LOCAL_PREV:
            //log(LOG_DEBUG, "thing 6\n");
            //log_tree(LOG_DEBUG, prev);
            //log_tree(LOG_DEBUG, curr_node);
            curr_node = prev->next;
            break;
        default:
            unreachable("");
    }
    //log_tree(LOG_DEBUG, curr_node);

    nodes_assert_tree_linkage_is_consistant(curr_node);

    Node* child = curr_node->left_child;
    while (child) {
        Node* child_next = child->next;
        node_printf(child_next);
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
