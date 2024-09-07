#ifndef NODES_H
#define NODES_H

#include <stddef.h>
#include <string.h>
#include <strings.h>
#include "util.h"
#include "node.h"
#include "assert.h"
#include "vector.h"

void nodes_log_tree_rec(LOG_LEVEL log_level, int pad_x, const Node* root, const char* file, int line);

void nodes_assert_tree_linkage_is_consistant(Node* root);

#define log_tree(log_level, root) \
    do { \
        nodes_assert_tree_linkage_is_consistant(root); \
        log_file_new(log_level, __FILE__, __LINE__, "tree:\n"); \
        nodes_log_tree_rec(log_level, 0, root, __FILE__, __LINE__); \
        log_file_new(log_level, __FILE__, __LINE__, "\n"); \
    } while(0)

// return prev if prev is non-null, or parent otherwise
static inline Node* nodes_prev_or_parent(Node* node) {
    if (!node->prev) {
        return node->parent;
    }
    return node->prev;
}

#define nodes_foreach_child(child, parent) \
    for (Node* child = (parent)->left_child; (child); (child) = (child)->next)

#define nodes_foreach_from_curr(curr_node, start_node) \
    for (Node* curr_node = (start_node); (curr_node); (curr_node) = (curr_node)->next)

#define nodes_foreach_from_curr_const(curr_node, start_node) \
    for (const Node* curr_node = (start_node); (curr_node); (curr_node) = (curr_node)->next)

#define nodes_foreach_from_curr_rev(curr_node, start_node) \
    for (Node* curr_node = (start_node); (curr_node); (curr_node) = (curr_node)->prev)

// do not use this function to remove node from tree (use node_remove instead for that)
static inline void nodes_reset_links_of_self_only(Node* node, bool keep_children) {
    if (!keep_children) {
        node->left_child = NULL;
    }
    node->next = NULL;
    node->prev = NULL;
    node->parent = NULL;
}

static inline Node* node_new(void) {
    Node* new_node = arena_alloc(sizeof(*new_node));
    nodes_reset_links_of_self_only(new_node, false);
    if (!root_of_tree) {
        root_of_tree = new_node;
    }
    return new_node;
}

static inline void nodes_establish_siblings(Node* curr, Node* next) {
    curr->next = next;
    next->prev = curr;
}

static inline size_t nodes_count_children(Node* parent) {
    size_t count = 0;
    nodes_foreach_child(child, parent) {
        count++;
    }
    return count;
}

static inline Node* nodes_get_leftmost_sibling(Node* node) {
    Node* result = node;
    while (result->prev) {
        result = result->prev;
    }
    return result;
}

static inline Node* nodes_get_local_leftmost(Node* start_node) {
    assert(start_node);

    Node* first;
    nodes_foreach_from_curr_rev(curr_node, start_node) {
        first = curr_node;
    }
    return first;
}

static inline void nodes_establish_parent_left_child(Node* parent, Node* child) {
    assert(parent && child);

    nodes_foreach_from_curr(curr_node, nodes_get_local_leftmost(child)) {
        child->parent = parent;
    }
    parent->left_child = child;
}

static inline void nodes_insert_after(Node* curr, Node* node_to_insert) {
    assert(curr && node_to_insert);

    Node* old_next = curr->next;
    nodes_establish_siblings(curr, node_to_insert);
    if (old_next) {
        nodes_establish_siblings(node_to_insert, old_next);
    }
    node_to_insert->parent = curr->parent;

    nodes_assert_tree_linkage_is_consistant(curr);
}

static inline void nodes_insert_before(Node* node_to_insert_before, Node* node_to_insert) {
    assert(node_to_insert_before && node_to_insert);

    Node* new_prev = node_to_insert_before->prev;
    Node* new_next = node_to_insert_before;
    if (new_prev) {
        nodes_establish_siblings(new_prev, node_to_insert);
    }

    assert(new_next);
    nodes_establish_siblings(node_to_insert, new_next);

    Node* parent = node_to_insert_before->parent;
    if (parent->left_child == node_to_insert_before) {
        parent->left_child = node_to_insert;
    }
    node_to_insert->parent = parent;

    //log_tree(LOG_DEBUG, node_id_from(0));
    //log_tree(LOG_DEBUG, parent);
    //log_tree(LOG_DEBUG, parent->left_child);
    //log_tree(LOG_DEBUG, node_to_insert);
}

static inline void nodes_append_child(Node* parent, Node* child) {
    // this function must not be used for appending nodes that have next, parent, or prev node(s) attached
    assert(!child->next);
    assert(!child->prev);
    assert(!child->parent);

    if (!parent->left_child) {
        parent->left_child = child;
        child->parent = parent;
        return;
    }

    Node* curr_node = parent->left_child;
    while (curr_node->next) {
        curr_node = curr_node->next;
    }
    nodes_insert_after(curr_node, child);

    nodes_assert_tree_linkage_is_consistant(parent);
}

// when node only has one child
static inline Node* nodes_single_child(Node* node) {
    assert(nodes_count_children(node) == 1);
    return node->left_child;
}

// child of src will be kept
// there should not be next, prev, or parent attached to src
// note: src will be modified and should probably not be used again
static inline void nodes_replace(Node* node_to_replace, Node* src) {
    assert(!src->next);
    assert(!src->prev);
    assert(!src->parent);
    assert(node_to_replace);

    if (!node_to_replace->prev) {
        nodes_establish_parent_left_child(node_to_replace->parent, src);
    } else {
        nodes_establish_siblings(node_to_replace->prev, src);
        src->parent = node_to_replace->parent;
    }

    if (node_to_replace->next) {
        nodes_establish_siblings(src, node_to_replace->next);
    }
}

static inline void nodes_remove_siblings(Node* node) {
    assert(node);

    Node* prev = node->prev;
    if (prev) {
        prev->next = NULL;
    }

    Node* next = node->next;
    if (next) {
        next->next = NULL;
    }

    node->next = NULL;
    node->prev = NULL;
}

static inline void nodes_remove_siblings_and_parent(Node* node) {
    nodes_remove_siblings(node);

    if (node->parent) {
        node->parent->left_child = NULL;
        node->parent = NULL;
    }
}

static inline Node* nodes_get_child_of_type(Node* parent, NODE_TYPE node_type) {
    if (!parent->left_child) {
        todo();
    }

    nodes_foreach_child(child, parent) {
        if (child->type == node_type) {
            return child;
        }
    }

    log_tree(LOG_VERBOSE, parent);
    unreachable("");
}

static inline Node* nodes_get_sibling_of_type(Node* node, NODE_TYPE node_type) {
    Node* curr_node = nodes_get_leftmost_sibling(node);

    while (!curr_node) {
        if (curr_node->type == node_type) {
            return curr_node;
        }
        curr_node = curr_node->next;
    }

    log_tree(LOG_VERBOSE, node);
    unreachable("");
}

static inline Node* nodes_get_child(Node* parent, size_t idx) {
    size_t curr_idx = 0;
    nodes_foreach_child(child, parent) {
        if (curr_idx == idx) {
            return child;
        }

        curr_idx++;
    }

    unreachable("");
}

static inline bool nodes_try_get_last_child_of_type(Node** result, Node* parent, NODE_TYPE node_type) {
    Node* curr_child = NULL;
    nodes_foreach_child(child, parent) {
        curr_child = child;
    }
    if (!curr_child) {
        return false;
    }

    nodes_foreach_from_curr_rev(curr, parent) {
        if (curr->type == node_type) {
            *result = curr;
            return true;
        }
    }

    return false;
}

static inline Node* nodes_get_last_child_of_type(Node* parent, NODE_TYPE node_type) {
    Node* result;
    try(nodes_try_get_last_child_of_type(&result, parent, node_type));
    return result;
}

// remove node_to_remove (and its children if present and keep_children is true) from tree. 
// children of node_to_remove will stay attached to node_to_remove if keep_children is true
static inline void nodes_remove(Node* node_to_remove, bool keep_children) {
    assert(node_to_remove);

    Node* next = node_to_remove->next;
    Node* prev = node_to_remove->prev;
    Node* parent = node_to_remove->parent;
    Node* left_child = node_to_remove->left_child;

    if (next) {
        next->prev = prev;
    }

    if (prev) {
        prev->next = next;
    }

    if (parent->left_child == node_to_remove) {
        parent->left_child = next;
    }

    if (!keep_children && left_child) {
        unreachable("");
    }

    nodes_reset_links_of_self_only(node_to_remove, keep_children);
}

static inline void nodes_extend_children(Node* parent, Node* start_of_nodes_to_extend) {
    Node* curr_node = start_of_nodes_to_extend;
    Node* next_node;
    while (1) {
        if (!curr_node) {
            break;
        }
        next_node = curr_node->next;

        nodes_remove(curr_node, true);
        nodes_append_child(parent, curr_node);
        nodes_assert_tree_linkage_is_consistant(parent);

        curr_node = next_node;
    }

    nodes_assert_tree_linkage_is_consistant(parent);
}

static inline Node* node_clone(const Node* node_to_clone) {
    Node* new_node = node_new();
    *new_node = *node_to_clone;
    nodes_reset_links_of_self_only(new_node, false);
    return new_node;
}

#endif // NODES_H
